#include "myz.h"
#include "ADTList.h"
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct {
    MyzNode node;
    char *filePath;
    bool gzip;
} ArchiveEntry;

static uint64_t next_node_id = 1; // Start from 1 (0 is root)

void destroy_archive_entry(void *value) {
    ArchiveEntry *entry = (ArchiveEntry *)value;
    free(entry->node.name);
    free(entry->node.link_target);
    free(entry->filePath);
    free(entry);
}

void add_to_list(List list, const MyzNode *node, const char *filePath, bool gzip) {
    ArchiveEntry *entry = malloc(sizeof(ArchiveEntry));
    if (!entry) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    entry->node = *node;
    entry->filePath = strdup(filePath);
    if (!entry->filePath) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    entry->gzip = gzip;
    list_insert_after(list, list_last(list), entry);
}

void write_metadata(int archive_fd, const MyzNode *node) {
    MyzNodeSerialized serialized = {
        .id = node->id,
        .parent_id = node->parent_id,
        .mode = node->mode,
        .uid = node->uid,
        .gid = node->gid,
        .mtime = node->mtime,
        .atime = node->atime,
        .ctime = node->ctime,
        .size = node->size,
        .data_offset = node->data_offset,
        .data_length = node->data_length,
        .name_len = node->name ? strlen(node->name) + 1 : 0,
        .link_target_len = node->link_target ? strlen(node->link_target) + 1 : 0,
        .inode = node->inode  // Add this
    };

    write(archive_fd, &serialized, sizeof(MyzNodeSerialized));

    if (node->name) {
        write(archive_fd, node->name, serialized.name_len);
    }
    if (node->link_target) {
        write(archive_fd, node->link_target, serialized.link_target_len);
    }
}

void write_file_data(int archive_fd, const char *filePath, bool gzip) {
    int fd = open(filePath, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    char buffer[4096];
    ssize_t bytesRead;

    if (gzip) {
        int gz_fd = dup(fd); // Duplicate to avoid closing the original
        gzFile gz = gzdopen(gz_fd, "rb");
        close(fd); // Close the original fd
        if (!gz) {
            perror("gzdopen");
            return;
        }

        while ((bytesRead = gzread(gz, buffer, sizeof(buffer))) > 0) {
            write(archive_fd, buffer, bytesRead);
        }

        gzclose(gz);
    } else {
        while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
            write(archive_fd, buffer, bytesRead);
        }
        close(fd);
    }
}

void process_directory(List list, const char *dirPath, bool gzip, uint64_t parent_id) {
    // Open the directory
    DIR *dir = opendir(dirPath);
    if (!dir) {
        perror("opendir");
        return;
    }

    // Read the directory entries
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the full path of the entry
        char *fullPath = malloc(strlen(dirPath) + strlen(entry->d_name) + 2);
        if (!fullPath) {
            perror("malloc");
            closedir(dir);
            return;
        }
        sprintf(fullPath, "%s/%s", dirPath, entry->d_name);

        
        struct stat st;
        if (lstat(fullPath, &st) == -1) {
            perror("lstat");
            free(fullPath);
            continue;
        }

        MyzNode node = {
            .id = next_node_id++,  // Assign unique ID
            .parent_id = parent_id,
            .name = strdup(entry->d_name),
            .mode = st.st_mode,
            .uid = st.st_uid,
            .gid = st.st_gid,
            .mtime = st.st_mtime,
            .atime = st.st_atime,
            .ctime = st.st_ctime,
            .size = st.st_size,
            .data_offset = 0,  // This will be set later
            .data_length = 0,  // This will be set later
            .link_target = NULL,
            .inode = st.st_ino
        };

        if (!node.name) {
            perror("strdup");
            free(fullPath);
            continue;
        }

        if (S_ISLNK(st.st_mode)) {
            char link_target[PATH_MAX];
            ssize_t len = readlink(fullPath, link_target, sizeof(link_target) - 1);
            if (len != -1) {
                link_target[len] = '\0';
                node.link_target = strdup(link_target);
                if (!node.link_target) {
                    perror("strdup");
                    free(node.name);
                    free(fullPath);
                    continue;
                }
            }
        }

        add_to_list(list, &node, fullPath, gzip);

        if (S_ISDIR(st.st_mode)) {
            process_directory(list, fullPath, gzip, node.id);
        }

        free(fullPath);
    }

    closedir(dir);
}

void compress_and_write_data(const char *filePath, FILE *archive) {
    int fd = open(filePath, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    char buffer[4096];
    ssize_t bytesRead;

    int gz_fd = dup(fd); // Duplicate to avoid closing the original
    gzFile gz = gzdopen(gz_fd, "rb");
    close(fd); // Close the original fd
    if (!gz) {
        perror("gzdopen");
        return;
    }

    while ((bytesRead = gzread(gz, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, bytesRead, archive);
    }

    gzclose(gz);
}

void write_metadata_section(FILE *archive, List list) {
    for (ListNode node = list_first(list); node != NULL; node = list_next(list, node)) {
        ArchiveEntry *entry = list_value(list, node);
        write_metadata(archive, &entry->node);
    }
}

void create_archive(const char *archiveFile, const char *filePath, bool gzip) {
    // Create a list to store archive entries
    List list = list_create(destroy_archive_entry);

    // Get file information for the specified file or directory
    struct stat st;
    if (lstat(filePath, &st) == -1) {
        perror("lstat");
        list_destroy(list);
        return;
    }
    
    // Duplicate the file path and extract the base name
    char *base = basename(strdup(filePath));
    if (!base) {
        perror("strdup");
        list_destroy(list);
        return;
    }
    
    // Initialize the root node for the archive with the base name and file metadata
    MyzNode rootNode = {
        .id = 0,                // Root node ID is 0
        .parent_id = 0,         // Root node has no parent
        .name = strdup(base),   // Set the name to the base name of the file or directory
        .mode = st.st_mode,     // File mode (permissions)
        .uid = st.st_uid,       // User ID of the owner
        .gid = st.st_gid,       // Group ID of the owner
        .mtime = st.st_mtime,   // Last modification time
        .atime = st.st_atime,   // Last access time
        .ctime = st.st_ctime,   // Creation time
        .size = st.st_size,     // Size of the file
        .data_offset = 0,       // Data offset (to be set later)
        .data_length = 0,       // Data length (to be set later)
        .link_target = NULL,    // Link target (if it's a symbolic link)
        .inode = st.st_ino      // Inode number
    };

    free(base);

    if (!rootNode.name) {
        perror("strdup");
        list_destroy(list);
        return;
    }

    add_to_list(list, &rootNode, filePath, gzip);

    if (S_ISDIR(st.st_mode)) {
        process_directory(list, filePath, gzip, rootNode.id);
    }

    int archive_fd = open(archiveFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (archive_fd == -1) {
        perror("open");
        list_destroy(list);
        return;
    }

    MyzHeader header = {
        .magic = "MYZ\0",
        .metadata_offset = 0,  // This will be set later
        .data_offset = sizeof(MyzHeader)
    };

    write(archive_fd, &header, sizeof(header));

    // Phase 1: Collect all data and record offsets
    off_t current_data_offset = lseek(archive_fd, 0, SEEK_CUR);
    for (ListNode node = list_first(list); node != NULL; node = list_next(list, node)) {
        ArchiveEntry *entry = list_value(list, node);
        if (S_ISREG(entry->node.mode)) {
            entry->node.data_offset = current_data_offset;
            write_file_data(archive_fd, entry->filePath, entry->gzip);
            entry->node.data_length = lseek(archive_fd, 0, SEEK_CUR) - entry->node.data_offset;
            current_data_offset = lseek(archive_fd, 0, SEEK_CUR);
        }
    }

    // Phase 2: Write metadata section
    header.metadata_offset = lseek(archive_fd, 0, SEEK_CUR);
    for (ListNode node = list_first(list); node != NULL; node = list_next(list, node)) {
        ArchiveEntry *entry = list_value(list, node);
        write_metadata(archive_fd, &entry->node);
    }

    // Update header.metadata_offset
    lseek(archive_fd, 0, SEEK_SET);
    write(archive_fd, &header, sizeof(header));

    close(archive_fd);
    list_destroy(list);
}

