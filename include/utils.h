#pragma once

#include "common.h"

void print_usage();
char **filter_paths(char **fileList, int *numFiles);

int parse_arguments(int argc, char *argv[], CommandLineArgs *args);