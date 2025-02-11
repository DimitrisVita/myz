#pragma once

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
    char *filePath;
} CommandLineArgs;