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
    uint64_t id;            // Unique identifier
    uint64_t parent_id;     // Parent node identifier
    ino_t inode;            // Inode number
    char *name;             // Name of the file or directory
    myz_node_type type;     // Type of the entry
    uid_t uid;              // User ID
    gid_t gid;              // Group ID
    mode_t mode;            // File mode
    time_t mtime, atime, ctime; // Timestamps
    off_t nodeSize;         // Size of the node
    off_t dataSize;         // Size of the data section
    off_t data_offset;      // Byte offset to the data section
    bool compressed;        // Data is compressed
} MyzNode;

void create_archive(const char *archiveFile, const char *filePath, bool gzip);
void append_archive(const char *archiveFile, const char *filePath, bool gzip);
void extract_archive(const char *archiveFile, const char *filePath);
void delete_archive(const char *archiveFile, const char *filePath);
void print_metadata(const char *archiveFile);
void query_archive(const char *archiveFile, const char *filePath);
void print_hierarchy(const char *archiveFile);