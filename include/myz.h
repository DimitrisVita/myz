#pragma once

#include "common.h"

typedef struct {
    char magic[4];  // Identifier "MYZ\0"
    uint64_t metadata_offset;  // Byte offset to the metadata section
} MyzHeader;

typedef struct {
    uint64_t id;
    uint64_t parent_id;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    time_t mtime, atime, ctime;
    off_t size;
    off_t data_offset;
    size_t data_length;
    uint32_t name_len;
    uint32_t link_target_len;
    uint64_t inode;  // Inode number for hard link detection
} MyzNodeSerialized;

typedef struct {
    uint64_t id;            // ID of the entry
    uint64_t parent_id;     // ID of the parent entry
    char *name;             // Name of the file or directory
    mode_t mode;            // File mode
    uid_t uid;              // User ID
    gid_t gid;              // Group ID
    time_t mtime, atime, ctime; // Timestamps
    off_t size;             // File size
    off_t data_offset;      // Byte offset to the data
    size_t data_length;     // Length of the data
    char *link_target;      // Target of the symbolic link
    uint64_t inode;         // Inode number for hard link detection
} MyzNode;

void create_archive(const char *archiveFile, const char *filePath, bool gzip);
void append_archive(const char *archiveFile, const char *filePath, bool gzip);
void extract_archive(const char *archiveFile, const char *filePath);
void delete_archive(const char *archiveFile, const char *filePath);
void print_metadata(const char *archiveFile);
void query_archive(const char *archiveFile, const char *filePath);
void print_hierarchy(const char *archiveFile);