#include "myz.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>  // Προσθήκη αυτής της γραμμής
#include <dirent.h>  // Προσθήκη αυτής της γραμμής

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
        node->type = S_ISDIR(st.st_mode) ? MYZ_NODE_TYPE_DIR : 
                      S_ISLNK(st.st_mode) ? MYZ_NODE_TYPE_SYMLINK : 
                      S_ISREG(st.st_mode) ? MYZ_NODE_TYPE_FILE : MYZ_NODE_TYPE_HARDLINK;
        node->data_offset = 0;    // This will be set later
        node->compressed = gzip;
        node->dirContents = (node->type == MYZ_NODE_TYPE_DIR) ? 0 : -1;
        if (node->type == MYZ_NODE_TYPE_FILE)
            *totalDataBytes += node->stat.st_size;

        // Add the node to the list
        list_insert_after(list, list_last(list), node);

        // Process the directory recursively
        if (node->type == MYZ_NODE_TYPE_DIR) {
            node->dirContents = processDirectory(fullPath, list, gzip, totalDataBytes);
        }

        // Increment the number of directory contents
        numDirContents++;
    }

    // Close the directory
    closedir(dir);

    return numDirContents;  // Return only the immediate children
}

// Function to create an archive
void compress_files_recursive(char *path) {
    struct stat st;
    if (lstat(path, &st) == -1) {
        perror("lstat");
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (dir == NULL) {
            perror("opendir");
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char fullPath[PATH_MAX];
            snprintf(fullPath, PATH_MAX, "%s/%s", path, entry->d_name);
            compress_files_recursive(fullPath);
        }

        closedir(dir);
    } else if (S_ISREG(st.st_mode)) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {  // Child
            execlp("gzip", "gzip", "-k", path, NULL);
            perror("exec");  // Only if exec fails
            exit(EXIT_FAILURE);
        } else {
            wait(NULL);  // Wait for the child process to finish
            // Update the size of the compressed file
            char gzPath[PATH_MAX];
            snprintf(gzPath, PATH_MAX, "%s.gz", path);
            struct stat gzSt;
            if (stat(gzPath, &gzSt) == 0) {
                st.st_size = gzSt.st_size;
            }
        }
    }
}

void compress_files(char **files) {
    for (int i = 0; files[i] != NULL; i++) {
        compress_files_recursive(files[i]);
    }

    while (wait(NULL) > 0);
}

void create_archive(char *archiveFile, char **fileList, bool gzip) {
    if (gzip) {
        compress_files(fileList);
    }

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
        // Update the path and name and the size of the file if it was compressed   
        if (gzip && S_ISREG(st.st_mode)) {
            char *dot = strrchr(node->name, '.');
            if (dot != NULL && strcmp(dot, ".gz") == 0) {
                *dot = '\0';    // Remove the .gz extension
            }
            snprintf(node->path, MAX_PATH_LEN, "%s.gz", fileList[i]);
            strncpy(node->name, basename(node->path), MAX_NAME_LEN - 1);

            struct stat gzSt;
            if (stat(node->path, &gzSt) == 0) {
                node->stat.st_size = gzSt.st_size;
            }
        }

        node->type = S_ISDIR(st.st_mode) ? MYZ_NODE_TYPE_DIR : MYZ_NODE_TYPE_FILE;  // Set the type
        node->data_offset = 0;    // This will be set later
        node->dirContents = (node->type == MYZ_NODE_TYPE_DIR) ? 0 : -1; // Set the number of directory contents
        node->compressed = gzip;

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

    // Decompress the file if it was compressed
    if (file_entry->compressed) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {  // Child
            execlp("gunzip", "gunzip", filePath, NULL);
            perror("exec");  // Only if exec fails
            exit(EXIT_FAILURE);
        } else {
            wait(NULL);  // Wait for the child process to finish
            // Update the size of the decompressed file
            struct stat st;
            if (stat(filePath, &st) == 0) {
                file_entry->stat.st_size = st.st_size;
            }
        }
    }
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

// Print the metadata of the archive
void print_metadata(char *archiveFile) {
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

    // Print the header information
    printf("=== Archive Header ===\n");
    printf("Magic: %s\n", header.magic);
    printf("Total bytes: %lu\n", header.total_bytes);
    printf("Metadata offset: %lu\n", header.metadata_offset);

    // Move the file descriptor to the metadata offset
    if (lseek(fd, header.metadata_offset, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        return;
    }

    // Read and print the list of archive entries
    MyzNode entry;
    printf("\n=== Archive Metadata ===\n");
    while (read(fd, &entry, sizeof(MyzNode)) == sizeof(MyzNode)) {
        printf("Name: %s\n", entry.name);
        printf("Path: %s\n", entry.path);
        printf("Type: %s\n", 
               entry.type == MYZ_NODE_TYPE_DIR ? "Directory" : 
               entry.type == MYZ_NODE_TYPE_FILE ? "File" : 
               entry.type == MYZ_NODE_TYPE_SYMLINK ? "Symlink" : 
               entry.type == MYZ_NODE_TYPE_HARDLINK ? "Hardlink" : "Unknown");
        printf("Data offset: %ld\n", entry.data_offset);
        printf("Size: %ld bytes\n", entry.stat.st_size);
        printf("Compressed: %s\n", entry.compressed ? "Yes" : "No");
        if (entry.type == MYZ_NODE_TYPE_DIR)
            printf("Number of directory contents: %d\n", entry.dirContents);

        // Print owner, group, and access rights
        printf("Owner: %d\n", entry.stat.st_uid);
        printf("Group: %d\n", entry.stat.st_gid);
        printf("Access rights: %o\n", entry.stat.st_mode & 0777);
        printf("\n");
    }

    // Close the archive file
    close(fd);
}

// Function to query an archive file, given the exact file path
void query_archive(char *archiveFile, char **fileList) {
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

    // Query the archive entries
    ListNode current = list_first(list);
    while (current != NULL) {
        MyzNode *current_entry = list_value(current);
        bool found = false;

        // Check if the current entry is in the list
        for (int i = 0; fileList[i] != NULL; i++) {
            if (strcmp(current_entry->path, fileList[i]) == 0) {
                found = true;
                break;
            }
        }

        // Print a positive or negative answer
        if (found) {
            printf("File '%s' found in the archive.\n", current_entry->path);
            break;
        } else {
            current = list_next(current);
        }
    }

    if (current == NULL) {
        printf("File not found in the archive.\n");
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

// Function to print the hierarchy of the archive
void print_hierarchy(char *archiveFile) {
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

    // Print the hierarchy of the archive
    printf("=== Archive Hierarchy ===\n");
    ListNode current = list_first(list);
    while (current != NULL) {
        MyzNode *current_entry = list_value(current);
        int depth = 0;

        // Print the hierarchy
        while (current_entry->type == MYZ_NODE_TYPE_DIR) {
            for (int i = 0; i < depth; i++) {
                printf("  ");
            }
            printf("%s/\n", current_entry->name);

            // Move to the next entry
            current = list_next(current);
            if (current == NULL) break;
            current_entry = list_value(current);
            depth++;
        }

        // Print the file
        for (int i = 0; i < depth; i++) {
            printf("  ");
        }
        printf("%s\n", current_entry->name);

        // Move to the next entry
        current = list_next(current);
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

// Function to check if a path exists in the archive
bool path_exists_in_archive(List list, const char *path) {
    ListNode node = list_first(list);
    while (node != NULL) {
        MyzNode *entry = list_value(node);
        if (strcmp(entry->path, path) == 0) {
            return true;
        }
        node = list_next(node);
    }
    return false;
}

// Function to filter paths, keeping the most generic path
void filter_paths_append(List list, char **fileList) {
    for (int i = 0; fileList[i] != NULL; i++) {
        ListNode node = list_first(list);
        ListNode prev = NULL;
        while (node != NULL) {
            MyzNode *entry = list_value(node);
            if (strncmp(entry->path, fileList[i], strlen(fileList[i])) == 0) {
                node = list_next(node);
                list_remove_after(list, prev);
                free(entry);
            } else {
                prev = node;
                node = list_next(node);
            }
        }
    }
}

// Function to append files and directories to an existing archive
void append_archive(char *archiveFile, char **fileList, bool gzip) {
    // Open the archive file
    int fd = open(archiveFile, O_RDWR);
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

    // Filter paths to remove specific files if their parent directory is added
    filter_paths_append(list, fileList);

    uint64_t totalDataBytes = 0;    // Total number of data bytes in the archive

    // Process each file and directory in the list
    for (int i = 0; fileList[i] != NULL; i++) {
        struct stat st; // File information
        if (lstat(fileList[i], &st) == -1) {
            perror("lstat");
            list_destroy(list);
            return;
        }

        // Check if the path already exists in the archive
        if (path_exists_in_archive(list, fileList[i])) {
            printf("Path '%s' already exists in the archive.\n", fileList[i]);
            continue;
        }

        // Check if the directory exists
        if (S_ISDIR(st.st_mode)) {
            DIR *dir = opendir(fileList[i]);
            if (dir == NULL) {
                perror("opendir");
                list_destroy(list);
                return;
            }
            closedir(dir);
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

    // Update the header of the archive
    header.total_bytes += sizeof(MyzNode) * list_size(list) + totalDataBytes;
    header.metadata_offset += totalDataBytes;

    // Transfer the list to the archive file
    int new_fd = transferListToFile(header, list, archiveFile);
    if (new_fd == -1) {
        list_destroy(list);
        return;
    }

    // Close the archive file
    close(new_fd);
    
    // Destroy the list
    list_destroy(list);
}

// Function to delete files and directories from an existing archive
void delete_archive(char *archiveFile, char **fileList) {
    // Open the archive file
    int fd = open(archiveFile, O_RDWR);
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

    // Remove the specified files and directories from the list
    for (int i = 0; fileList[i] != NULL; i++) {
        ListNode node = list_first(list);
        ListNode prev = NULL;
        while (node != NULL) {
            MyzNode *entry = list_value(node);
            if (strcmp(entry->path, fileList[i]) == 0) {
                // Update the parent directory's dirContents
                ListNode parentNode = list_first(list);
                while (parentNode != NULL) {
                    MyzNode *parentEntry = list_value(parentNode);
                    if (parentEntry->type == MYZ_NODE_TYPE_DIR && 
                        strncmp(entry->path, parentEntry->path, strlen(parentEntry->path)) == 0) {
                        parentEntry->dirContents--;
                        break;
                    }
                    parentNode = list_next(parentNode);
                }

                // If the entry is a directory, remove its contents
                if (entry->type == MYZ_NODE_TYPE_DIR) {
                    ListNode childNode = list_next(node);
                    while (childNode != NULL) {
                        MyzNode *childEntry = list_value(childNode);
                        if (strncmp(childEntry->path, entry->path, strlen(entry->path)) == 0) {
                            ListNode nextChildNode = list_next(childNode);
                            list_remove_after(list, node);
                            free(childEntry);
                            childNode = nextChildNode;
                        } else {
                            break;
                        }
                    }
                }

                node = list_next(node);
                list_remove_after(list, prev);
                free(entry);
            } else {
                prev = node;
                node = list_next(node);
            }
        }
    }

    // Update the header of the archive
    uint64_t totalDataBytes = 0;
    ListNode node = list_first(list);
    while (node != NULL) {
        MyzNode *entry = list_value(node);
        if (entry->type == MYZ_NODE_TYPE_FILE) {
            totalDataBytes += entry->stat.st_size;
        }
        node = list_next(node);
    }
    header.total_bytes = sizeof(MyzHeader) + sizeof(MyzNode) * list_size(list) + totalDataBytes;
    header.metadata_offset = sizeof(MyzHeader) + totalDataBytes;

    // Transfer the updated list to the archive file
    int new_fd = transferListToFile(header, list, archiveFile);
    if (new_fd == -1) {
        list_destroy(list);
        return;
    }

    // Close the archive file
    close(new_fd);

    // Destroy the list
    list_destroy(list);
}