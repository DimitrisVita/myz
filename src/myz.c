#include "myz.h"

// Transfer the list to the archive file and return the file descriptor
int transferListToFile(MyzHeader header, List list, char *archiveFile) {
    // Open the archive file
    int fd = open(archiveFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    // Write the header to the archive file
    if (write(fd, &header, sizeof(MyzHeader)) == -1) {
        perror("write");
        close(fd);
        return -1;
    }

    int metadataBytesWritten = 0;
    int dataBytesWritten = 0;

    // Write the list of archive entries to the archive file
    ListNode node = list_first(list);
    while (node != NULL) {
        MyzNode *entry = list_value(list, node);

        if (entry->type == MYZ_NODE_TYPE_FILE) {
            // Open the file to read its data
            int file_fd = open(entry->path, O_RDONLY);
            if (file_fd == -1) {
                perror("open");
                close(fd);
                return -1;
            }

            // Copy the data from the file to the archive file
            char buffer[1024];
            ssize_t bytesRead;
            while ((bytesRead = read(file_fd, buffer, sizeof(buffer))) > 0) {
                if (write(fd, buffer, bytesRead) == -1) {
                    perror("write");
                    close(fd);
                    close(file_fd);
                    return -1;
                }
                dataBytesWritten += bytesRead;
            }

            // Close the file
            close(file_fd);

            // Calculate the data offset
            entry->data_offset = lseek(fd, 0, SEEK_CUR) - entry->stat.st_size;

            // Move the fd to the metadata section. We put the metadata at the end of the archive file
            if (lseek(fd, header.metadata_offset + metadataBytesWritten, SEEK_SET) == -1) {
                perror("lseek");
                close(fd);
                return -1;
            }

            // Write the updated metadata (entry) to the archive file
            if (write(fd, entry, sizeof(MyzNode)) == -1) {
                perror("write");
                close(fd);
                return -1;
            }
            metadataBytesWritten += sizeof(MyzNode);

        } else if (entry->type == MYZ_NODE_TYPE_DIR) {
            // Write the directory metadata to the archive file
            if (write(fd, entry, sizeof(MyzNode)) == -1) {
                perror("write");
                close(fd);
                return -1;
            }
            metadataBytesWritten += sizeof(MyzNode);
        }

        node = list_next(list, node);
    }

    return fd;
}


// Process the directory recursively
int processDirectory(char *dirPath, List list, bool gzip, uint64_t *totalDataBytes) {
    // Open the directory
    DIR *dir = opendir(dirPath);
    if (dir == NULL) {
        perror("opendir");
        return 0;
    }

    // Initialize the number of directory contents
    int numDirContents = 0;

    // Process each file and directory in the directory
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip the current and parent directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Get the full path of the file or directory
        char fullPath[PATH_MAX];
        snprintf(fullPath, PATH_MAX, "%s/%s", dirPath, entry->d_name);

        // Get file information
        struct stat st;
        if (lstat(fullPath, &st) == -1) {
            perror("lstat");
            continue;
        }

        // Initialize the node for the file or directory and allocate memory dynamically
        MyzNode *node = malloc(sizeof(MyzNode));
        node->stat = st;
        node->name = malloc(strlen(entry->d_name) + 1); strcpy(node->name, entry->d_name);
        node->path = malloc(strlen(fullPath) + 1); strcpy(node->path, fullPath);
        node->type = S_ISDIR(st.st_mode) ? MYZ_NODE_TYPE_DIR : MYZ_NODE_TYPE_FILE;
        node->data_offset = 0;    // This will be set later
        node->compressed = gzip;
        node->dirContents = (node->type == MYZ_NODE_TYPE_DIR) ? 0 : -1;

        // Add the file size to the total bytes
        if (node->type == MYZ_NODE_TYPE_FILE) {
            *totalDataBytes += node->stat.st_size;
        }

        // Add the node to the list
        list_insert_after(list, list_last(list), node);

        // Process the directory recursively
        if (node->type == MYZ_NODE_TYPE_DIR) {
            numDirContents += processDirectory(fullPath, list, gzip, totalDataBytes);
        }

        // Increment the number of directory contents
        numDirContents++;

    }

    // Close the directory
    closedir(dir);

    return numDirContents;
}



void create_archive(char *archiveFile, char **fileList, bool gzip) {
    // Create a list to store archive entries
    List list = list_create(NULL);

    uint64_t totalDataBytes = 0;

    // Process each file and directory in the list
    for (int i = 0; fileList[i] != NULL; i++) {
        // Get file information
        struct stat st;
        if (lstat(fileList[i], &st) == -1) {
            perror("lstat");
            list_destroy(list);
            return;
        }

        // Initialize the node for the file or directory and allocate memory dynamically
        MyzNode *node = malloc(sizeof(MyzNode));
        node->stat = st;
        node->name = malloc(strlen(fileList[i]) + 1); strcpy(node->name, fileList[i]);
        node->path = malloc(strlen(fileList[i]) + 1); strcpy(node->path, fileList[i]);
        node->type = S_ISDIR(st.st_mode) ? MYZ_NODE_TYPE_DIR : MYZ_NODE_TYPE_FILE;
        node->data_offset = 0;    // This will be set later
        node->dirContents = (node->type == MYZ_NODE_TYPE_DIR) ? 0 : -1;

        // Add the file size to the total bytes
        if (node->type == MYZ_NODE_TYPE_FILE) {
            totalDataBytes += node->stat.st_size;
        }

        // Add the node to the list
        list_insert_after(list, list_last(list), node);

        // Process the directory recursively
        if (node->type == MYZ_NODE_TYPE_DIR) {
            node->dirContents = processDirectory(fileList[i], list, gzip, &totalDataBytes);
        }

    }

    // Initialize the header of the archive
    MyzHeader header = {
        "MYZ",
        sizeof(MyzHeader) + sizeof(MyzNode) * list_size(list) + totalDataBytes,
        sizeof(MyzHeader) + totalDataBytes
    };

    // Print the list of archive entries
    ListNode node = list_first(list);
    while (node != NULL) {
        MyzNode *entry = list_value(list, node);
        
        printf("Name: %s, Type: %d\n", entry->name, entry->type);
        printf("Mode: %o\n", entry->stat.st_mode);
        printf("UID: %d\n", entry->stat.st_uid);
        printf("GID: %d\n", entry->stat.st_gid);
        printf("Size: %ld\n", entry->stat.st_size);
        printf("Access Time: %ld\n", entry->stat.st_atime);
        printf("Modification Time: %ld\n", entry->stat.st_mtime);
        printf("Change Time: %ld\n", entry->stat.st_ctime);
        printf("Path: %s\n", entry->path);
        printf("Data Offset: %ld\n", entry->data_offset);
        printf("Compressed: %d\n", entry->compressed);
        printf("Directory Contents: %d\n\n", entry->dirContents);

        node = list_next(list, node);
    }

    // Transfer the list to the archive file
    int fd = transferListToFile(header, list, archiveFile);
    if (fd == -1) {
        list_destroy(list);
        return;
    }

    // Close the archive file
    close(fd);
    
    // Free dynamically allocated memory
    node = list_first(list);
    while (node != NULL) {
        MyzNode *entry = list_value(list, node);
        free(entry->name);
        free(entry->path);
        free(entry);
        node = list_next(list, node);
    }

    // Destroy the list
    list_destroy(list);
}

// void append_archive(char *archiveFile, char *filePath, bool gzip) {
//     // Function definition
// }

// void extract_archive(char *archiveFile, char *filePath) {
//     // Function definition
// }

// void delete_archive(char *archiveFile, char *filePath) {
//     // Function definition
// }

// void print_metadata(char *archiveFile) {
//     // Function definition
// }

// void query_archive(char *archiveFile, char *filePath) {
//     // Function definition
// }

// void print_hierarchy(char *archiveFile) {
//     // Function definition
// }