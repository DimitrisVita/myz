#pragma once

#include "common.h"
#include "ADTList.h"

// Header of the archive
typedef struct {
    char magic[4];  // Identifier "MYZ\0"
    uint64_t total_bytes;   // Total bytes of the archive
    uint64_t metadata_offset;  // Byte offset to the metadata section
} MyzHeader;

typedef enum {
    MYZ_NODE_TYPE_FILE,     // Regular file
    MYZ_NODE_TYPE_DIR,      // Directory
    MYZ_NODE_TYPE_SYMLINK,  // Symbolic link
    MYZ_NODE_TYPE_HARDLINK  // Hard link
} myz_node_type;

// Metadata node
typedef struct {
    ino_t inode;            // Inode number
    char *name;             // Name of the file or directory
    char *path;             // Full path of the file or directory
    myz_node_type type;     // Type of the entry
    uid_t uid;              // User ID
    gid_t gid;              // Group ID
    mode_t mode;            // File mode
    time_t mtime, atime, ctime; // Timestamps
    off_t dataSize;         // Size of the data section
    off_t data_offset;      // Byte offset to the data section
    bool compressed;        // Data is compressed
    int dirContents;        // Number of directory contents
} MyzNode;

void create_archive(char *archiveFile, char **fileList, bool gzip);
void append_archive(char *archiveFile, char **fileList, bool gzip);
void extract_archive(char *archiveFile, char **fileList);
void delete_archive(char *archiveFile, char **fileList);
void print_metadata(char *archiveFile);
void query_archive(char *archiveFile, char **fileList);
void print_hierarchy(char *archiveFile);