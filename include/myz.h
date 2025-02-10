#pragma once

#include "common.h"

void create_archive(const char *archiveFile, const char *filePath, bool gzip);
void append_archive(const char *archiveFile, const char *filePath, bool gzip);
void extract_archive(const char *archiveFile, const char *filePath);
void delete_archive(const char *archiveFile, const char *filePath);
void print_metadata(const char *archiveFile);
void query_archive(const char *archiveFile, const char *filePath);
void print_hierarchy(const char *archiveFile);