#pragma once

#define _XOPEN_SOURCE 500 // For lstat

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <zlib.h>
#include <libgen.h>
#include <linux/limits.h>

#define MAX_NAME_LEN 256
#define MAX_PATH_LEN 1024

// Structure to hold command line arguments
typedef struct {
    bool create;
    bool append;
    bool export;
    bool metadata;
    bool delete;
    bool print;
    bool query;
    bool gzip;
    char *archiveFile;
    char **fileList;
    int numFiles;
} CommandLineArgs;