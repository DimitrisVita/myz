#include "myz.h"

// Process the directory recursively
int processDirectory(const char *dirPath, List list, int parent_id, bool gzip) {
    // Open the directory
    DIR *dir = opendir(dirPath);
    if (dir == NULL) {
        perror("opendir");
        return 0;
    }

    // Initialize the number of directory contents
    int numDirContents = 0;

    // Read the directory entries
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip the "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Get the full path of the directory entry
        char entryPath[PATH_MAX];
        snprintf(entryPath, PATH_MAX, "%s/%s", dirPath, entry->d_name);

        // Get file information for the directory entry
        struct stat st;
        if (lstat(entryPath, &st) == -1) {
            perror("lstat");
            continue;
        }

        // Initialize the node for the directory entry
        MyzNode node = {
            .inode = st.st_ino,
            .id = 0,    // This will be set later
            .parent_id = parent_id,
            .name = strdup(entry->d_name),
            .type = S_ISDIR(st.st_mode) ? MYZ_NODE_TYPE_DIR : MYZ_NODE_TYPE_FILE,
            .uid = st.st_uid,
            .gid = st.st_gid,
            .mode = st.st_mode,
            .mtime = st.st_mtime,
            .atime = st.st_atime,
            .ctime = st.st_ctime,
            .nodeSize = sizeof(MyzNode),
            .dataSize = 0,  // This will be set later
            .data_offset = 0    // This will be set later
        };

        // Add the node to the list
        add_to_list(list, &node, entryPath, gzip);

        // Process the directory recursively
        if (node.type == MYZ_NODE_TYPE_DIR) {
            numDirContents += processDirectory(entryPath, list, node.id, gzip);
        }

        // Increment the number of directory contents
        numDirContents++;
    }

    // Close the directory
    closedir(dir);

    return numDirContents;
}



void create_archive(const char *archiveFile, const char *filePath, bool gzip) {
    // Create a list to store archive entries
    List list = list_create(NULL);

    // Get file information for the specified file or directory
    struct stat st;
    if (lstat(filePath, &st) == -1) {
        perror("lstat");
        list_destroy(list);
        return;
    }
    
    // Initialize the root node for the archive with the base name and file metadata
    MyzNode rootNode = {
        .inode = st.st_ino,
        .id = 0,    // Root node ID is 0
        .parent_id = 0, // Root node has no parent
        .name = basename(strdup(filePath)), // Get the base name of the file path
        .type = S_ISDIR(st.st_mode) ? MYZ_NODE_TYPE_DIR : MYZ_NODE_TYPE_FILE,
        .uid = st.st_uid,
        .gid = st.st_gid,
        .mode = st.st_mode,
        .mtime = st.st_mtime,
        .atime = st.st_atime,
        .ctime = st.st_ctime,
        .nodeSize = sizeof(MyzNode),
        .dataSize = 0,  // This will be set later
        .data_offset = 0    // This will be set later
    };

    add_to_list(list, &rootNode, filePath, gzip);

    // Process the directory recursively
    int numDirContents = 0; // Number of directory contents
    if (rootNode.type == MYZ_NODE_TYPE_DIR) {
        numDirContents = processDirectory(filePath, list, 1, gzip);
    }

    // // Open the archive file for writing
    // int archive_fd = open(archiveFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    // if (archive_fd == -1) {
    //     perror("open");
    //     list_destroy(list);
    //     return;
    // }

    // // Calculate the byte offset for the metadata section
    // off_t metadata_offset = calculate_metadata_offset(list, numDirContents);

    // // Calculate the total bytes of the archive
    // uint64_t total_bytes = calculate_total_bytes(list, metadata_offset);

    // // Initialize the header of the archive
    // MyzHeader header = {
    //     .magic = "MYZ",
    //     .total_bytes = 0,   // This will be set later
    //     .metadata_offset = 0    // This will be set later
    // };

    // // Write the header to the archive file
    // write_header(archive_fd, &header);

    // // Write the data sections to the archive file
    // write_data_sections(archive_fd, list, gzip);

    // // Write the metadata sections to the archive file
    // write_metadata_sections(archive_fd, list, metadata_offset, gzip);
    
    // // Update the header with the total bytes and metadata offset
    // header.total_bytes = total_bytes;
    // header.metadata_offset = metadata_offset;

    // // Write the updated header to the archive file
    // write_header(archive_fd, &header);

    // // Close the archive file
    // close(archive_fd);

    // Destroy the list
    list_destroy(list);
}

void append_archive(const char *archiveFile, const char *filePath, bool gzip) {
    // Function definition
}

void extract_archive(const char *archiveFile, const char *filePath) {
    // Function definition
}

void delete_archive(const char *archiveFile, const char *filePath) {
    // Function definition
}

void print_metadata(const char *archiveFile) {
    // Function definition
}

void query_archive(const char *archiveFile, const char *filePath) {
    // Function definition
}

void print_hierarchy(const char *archiveFile) {
    // Function definition
}