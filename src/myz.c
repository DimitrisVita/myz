#include "myz.h"
#include "ADTList.h"
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>




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
    if (rootNode.type == MYZ_NODE_TYPE_DIR) {
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
        .total_bytes = 0,  // This will be set later
        .metadata_offset = 0  // This will be set later
    };

}