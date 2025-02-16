#include "myz.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// Function to transfer the list of archive entries to the archive file
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

    int metadataBytesWritten = 0;   // Number of metadata bytes written
    int dataBytesWritten = 0;       // Number of data bytes written

    // Write the list of archive entries to the archive file
    ListNode node = list_first(list);
    while (node != NULL) {
        MyzNode *entry = list_value(node);

        if (entry->type == MYZ_NODE_TYPE_FILE) {
            // Open the file to read its data
            int file_fd = open(entry->path, O_RDONLY);
            if (file_fd == -1) {
                perror("open");
                close(fd);
                return -1;
            }

            // Calculate the data offset
            entry->data_offset = sizeof(MyzHeader) + dataBytesWritten;

            // Move the fd to the data section
            if (lseek(fd, entry->data_offset, SEEK_SET) == -1) {
                perror("lseek");
                close(fd);
                close(file_fd);
                return -1;
            }

            // Copy the data from the file to the archive file
            char buffer[1024];
            ssize_t bytesRead;
            size_t bytesToRead = entry->stat.st_size;
            while (bytesToRead > 0) {
                int min = bytesToRead < sizeof(buffer) ? bytesToRead : sizeof(buffer);
                bytesRead = read(file_fd, buffer, min);
                if (bytesRead <= 0) {
                    perror("read");
                    close(fd);
                    close(file_fd);
                    return -1;
                }

                if (write(fd, buffer, bytesRead) == -1) {
                    perror("write");
                    close(fd);
                    close(file_fd);
                    return -1;
                }
                bytesToRead -= bytesRead;
                dataBytesWritten += bytesRead;
            }

            // Close the file
            close(file_fd);

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
            metadataBytesWritten += sizeof(MyzNode);    // Increment the number of metadata bytes written

        } else if (entry->type == MYZ_NODE_TYPE_DIR) {
            // Move the fd to the metadata section. We put the metadata at the end of the archive file
            if (lseek(fd, header.metadata_offset + metadataBytesWritten, SEEK_SET) == -1) {
                perror("lseek");
                close(fd);
                return -1;
            }

            // Write the directory metadata to the archive file
            if (write(fd, entry, sizeof(MyzNode)) == -1) {
                perror("write");
                close(fd);
                return -1;
            }
            metadataBytesWritten += sizeof(MyzNode);
        }

        node = list_next(node);
    }

    return fd;
}


// Function to process a directory recursively and return the number of directory contents
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
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

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
        memset(node, 0, sizeof(MyzNode)); // Initialize memory to zero
        node->stat = st;
        strncpy(node->name, entry->d_name, MAX_NAME_LEN - 1);
        node->name[MAX_NAME_LEN - 1] = '\0';
        strncpy(node->path, fullPath, MAX_PATH_LEN - 1);
        node->path[MAX_PATH_LEN - 1] = '\0';
        node->type = S_ISDIR(st.st_mode) ? MYZ_NODE_TYPE_DIR : MYZ_NODE_TYPE_FILE;
        node->data_offset = 0;    // This will be set later
        node->compressed = gzip;
        node->dirContents = (node->type == MYZ_NODE_TYPE_DIR) ? 0 : -1;

        // Add the file size to the total bytes
        if (node->type == MYZ_NODE_TYPE_FILE)
            *totalDataBytes += node->stat.st_size;

        // Add the node to the list
        list_insert_after(list, list_last(list), node);

        // Process the directory recursively
        if (node->type == MYZ_NODE_TYPE_DIR) {
            int subDirContents = processDirectory(fullPath, list, gzip, totalDataBytes);
            node->dirContents = subDirContents;
            numDirContents += subDirContents;
        }

        // Increment the number of directory contents
        numDirContents++;
    }

    // Close the directory
    closedir(dir);

    return numDirContents;
}

// Function to create an archive
void create_archive(char *archiveFile, char **fileList, bool gzip) {
    List list = list_create(NULL);  // List to store the file and directory information

    uint64_t totalDataBytes = 0;    // Total number of data bytes in the archive

    // Process each file and directory in the list
    for (int i = 0; fileList[i] != NULL; i++) {
        struct stat st; // File information
        if (lstat(fileList[i], &st) == -1) {
            perror("lstat");
            list_destroy(list);
            return;
        }

        // Initialize the node for the file or directory
        MyzNode *node = malloc(sizeof(MyzNode));
        memset(node, 0, sizeof(MyzNode)); // Initialize memory to zero
        node->stat = st;    // Copy the file information
        
        // Get the name of the file or directory
        char *lastPart = strrchr(fileList[i], '/');
        if (lastPart == NULL) { // No directory part
            strncpy(node->name, fileList[i], MAX_NAME_LEN - 1);
            node->name[MAX_NAME_LEN - 1] = '\0';
        } else {    // Directory part exists
            strncpy(node->name, lastPart + 1, MAX_NAME_LEN - 1);
            node->name[MAX_NAME_LEN - 1] = '\0';
        }
        strncpy(node->path, fileList[i], MAX_PATH_LEN - 1); // Copy the path
        node->path[MAX_PATH_LEN - 1] = '\0';
        node->type = S_ISDIR(st.st_mode) ? MYZ_NODE_TYPE_DIR : MYZ_NODE_TYPE_FILE;  // Set the type
        node->data_offset = 0;    // This will be set later
        node->dirContents = (node->type == MYZ_NODE_TYPE_DIR) ? 0 : -1; // Set the number of directory contents

        // Add the file size to the total bytes
        if (node->type == MYZ_NODE_TYPE_FILE)
            totalDataBytes += node->stat.st_size;

        // Add the node to the list
        list_insert_after(list, list_last(list), node);

        // Process the directory recursively
        if (node->type == MYZ_NODE_TYPE_DIR)
            node->dirContents = processDirectory(fileList[i], list, gzip, &totalDataBytes);
    }

    // Initialize the header of the archive
    MyzHeader header = {
        "MYZ",
        sizeof(MyzHeader) + sizeof(MyzNode) * list_size(list) + totalDataBytes,
        sizeof(MyzHeader) + totalDataBytes
    };

    // Transfer the list to the archive file
    int fd = transferListToFile(header, list, archiveFile);
    if (fd == -1) {
        list_destroy(list);
        return;
    }

    // Close the archive file
    close(fd);
    
    // Free dynamically allocated memory
    ListNode node = list_first(list);
    while (node != NULL) {
        MyzNode *entry = list_value(node);
        free(entry);
        node = list_next(node);
    }

    // Destroy the list
    list_destroy(list);
}

// Function to extract an archive
void extract_file(int fd, MyzNode *file_entry, const char *basePath) {
    // Remove any leading "./" from the base path
    while (strncmp(basePath, "./", 2) == 0) {
        basePath += 2;
    }

    // Remove any leading "./" from the file name as well
    const char *fileName = file_entry->name;
    while (strncmp(fileName, "./", 2) == 0) {
        fileName += 2;
    }

    // Construct the file path using the cleaned file name
    char filePath[PATH_MAX];
    snprintf(filePath, sizeof(filePath), "%s/%s", basePath, fileName);

    // Check if the file already exists and append a suffix if necessary
    int suffix = 1;
    char originalFilePath[PATH_MAX];
    strcpy(originalFilePath, filePath);
    char *dot = strrchr(originalFilePath, '.');
    while (access(filePath, F_OK) == 0) {
        if (dot) {
            int prefix_length = (int)(dot - originalFilePath);
            snprintf(filePath, sizeof(filePath), "%s/%.*s(%d)%s",
                     basePath, prefix_length, originalFilePath, suffix++, dot);
        } else {
            snprintf(filePath, sizeof(filePath), "%s/%s(%d)",
                     basePath, fileName, suffix++);
        }
    }

    // Remove any leading "./" from the file path before printing
    const char *cleanFilePath = filePath;
    while (strncmp(cleanFilePath, "./", 2) == 0) {
        cleanFilePath += 2;
    }

    // Print the original archive path
    printf("'%s' original archive path: '%s'\n", cleanFilePath, file_entry->path);

    int file_fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, file_entry->stat.st_mode);

    lseek(fd, file_entry->data_offset, SEEK_SET);   // Move to the data offset

    // Read the data from the archive and write it to the file
    size_t bytesToRead = file_entry->stat.st_size;
    char buffer[1024];
    while (bytesToRead > 0) {
        int min = bytesToRead < sizeof(buffer) ? bytesToRead : sizeof(buffer);
        ssize_t bytesRead = read(fd, buffer, min);
        if (bytesRead <= 0) break;
        write(file_fd, buffer, bytesRead);
        bytesToRead -= bytesRead;
    }
    close(file_fd);
}


// Function to extract an archive
void extract_directory(int fd, List list, ListNode *current, const char *basePath) {
    // Create the directory
    MyzNode *dir_entry = list_value(*current);
    char dirPath[PATH_MAX];
    snprintf(dirPath, sizeof(dirPath), "%s/%s", basePath, dir_entry->name);

    if (mkdir(dirPath, dir_entry->stat.st_mode) == -1 && errno != EEXIST) {
        perror("mkdir");
        return;
    }

    // Extract the directory contents
    int numChildren = dir_entry->dirContents;
    *current = list_next(*current);

    // Extract each child
    for (int i = 0; i < numChildren; ++i) {
        MyzNode *child = list_value(*current);
        if (child->type == MYZ_NODE_TYPE_DIR) {
            extract_directory(fd, list, current, dirPath);
        } else {
            extract_file(fd, child, dirPath);
            *current = list_next(*current);
        }
    }
}

void extract_archive(char *archiveFile, char **fileList) {
    // Open the archive file
    int fd = open(archiveFile, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    // Read the header of the archive
    MyzHeader header;
    if (read(fd, &header, sizeof(MyzHeader)) != sizeof(MyzHeader)) {
        perror("read");
        close(fd);
        return;
    }

    // Check if the archive file is valid
    if (strncmp(header.magic, "MYZ", 4) != 0) {
        fprintf(stderr, "Invalid archive file\n");
        close(fd);
        return;
    }

    // Move the file descriptor to the metadata offset
    if (lseek(fd, header.metadata_offset, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        return;
    }

    // Read the list of archive entries
    List list = list_create(NULL);
    MyzNode entry;
    while (read(fd, &entry, sizeof(MyzNode)) == sizeof(MyzNode)) {
        MyzNode *node = malloc(sizeof(MyzNode));
        memset(node, 0, sizeof(MyzNode)); // Initialize memory to zero
        *node = entry;

        list_insert_after(list, list_last(list), node);
    }

    // Extract the files and directories
    ListNode current = list_first(list);
    while (current != NULL) {
        MyzNode *current_entry = list_value(current);
        bool extract = true;

        // If a file list is provided, check if the current entry is in the list
        if (fileList != NULL && fileList[0] != NULL) {
            extract = false;
            for (int i = 0; fileList[i] != NULL; i++) {
                if (strcmp(current_entry->path, fileList[i]) == 0) {
                    extract = true;
                    break;
                }
            }
        }

        if (extract) {
            if (current_entry->type == MYZ_NODE_TYPE_DIR) {
                extract_directory(fd, list, &current, ".");
            } else {
                extract_file(fd, current_entry, ".");
                current = list_next(current);
            }
        } else {
            current = list_next(current);
        }
    }

    // Close the archive file
    close(fd);

    // Free dynamically allocated memory
    ListNode node = list_first(list);
    while (node != NULL) {
        MyzNode *entry = list_value(node);
        free(entry);
        node = list_next(node);
    }

    // Destroy the list
    list_destroy(list);
}