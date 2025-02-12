#include "myz.h"

// Transfer the list to the archive file


// Process the directory recursively
int processDirectory(char *dirPath, List list, bool gzip) {
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
        node->inode = st.st_ino;
        node->name = malloc(strlen(entry->d_name) + 1); strcpy(node->name, entry->d_name);
        node->path = malloc(strlen(fullPath) + 1); strcpy(node->path, fullPath);
        node->type = S_ISDIR(st.st_mode) ? MYZ_NODE_TYPE_DIR : MYZ_NODE_TYPE_FILE;
        node->uid = st.st_uid;
        node->gid = st.st_gid;
        node->mode = st.st_mode;
        node->mtime = st.st_mtime;
        node->atime = st.st_atime;
        node->ctime = st.st_ctime;
        node->dataSize = (node->type == MYZ_NODE_TYPE_FILE) ? st.st_size : 0;
        node->data_offset = 0;    // This will be set later
        node->compressed = gzip;

        // Add the node to the list
        list_insert_after(list, list_last(list), node);

        // Process the directory recursively
        if (node->type == MYZ_NODE_TYPE_DIR) {
            numDirContents += processDirectory(fullPath, list, gzip);
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
        node->inode = st.st_ino;
        node->name = malloc(strlen(fileList[i]) + 1); strcpy(node->name, fileList[i]);
        node->path = malloc(strlen(fileList[i]) + 1); strcpy(node->path, fileList[i]);
        node->type = S_ISDIR(st.st_mode) ? MYZ_NODE_TYPE_DIR : MYZ_NODE_TYPE_FILE;
        node->uid = st.st_uid;
        node->gid = st.st_gid;
        node->mode = st.st_mode;
        node->mtime = st.st_mtime;
        node->atime = st.st_atime;
        node->ctime = st.st_ctime;
        node->dataSize = (node->type == MYZ_NODE_TYPE_FILE) ? st.st_size : 0;
        node->data_offset = 0;    // This will be set later

        // Add the node to the list
        list_insert_after(list, list_last(list), node);

        // Process the directory recursively
        if (node->type == MYZ_NODE_TYPE_DIR) {
            node->dirContents = processDirectory(fileList[i], list, gzip);
        }

    }

    // Print the list of archive entries
    ListNode node = list_first(list);
    while (node != NULL) {
        MyzNode *entry = list_value(list, node);
        printf("Name: %s, Type: %d\n", entry->name, entry->type);
        
        // Print all metadata
        printf("Inode: %lu\n", entry->inode);
        printf("Path: %s\n", entry->path);
        printf("UID: %u, GID: %u, Mode: %o\n", entry->uid, entry->gid, entry->mode);
        printf("Data Size: %ld\n", entry->dataSize);
        printf("Data Offset: %ld, Compressed: %d\n", entry->data_offset, entry->compressed);
        printf("Timestamps: %ld, %ld, %ld\n", entry->mtime, entry->atime, entry->ctime);
        printf("Directory Contents: %d\n\n", entry->dirContents);


        node = list_next(list, node);
    }

    // Transfer the list to the archive file
    
    
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