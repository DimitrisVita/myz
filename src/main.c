#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myz.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s {-c|-a|-x|-m|-d|-p|-j} <archive-file> <list-of-files/dirs>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *flag = argv[1];
    char *archive_file = argv[2];
    char **files_dirs = &argv[3];
    int num_files_dirs = argc - 3;

    if (strcmp(flag, "-c") == 0) {
        create_archive(archive_file, files_dirs, num_files_dirs);
    } else if (strcmp(flag, "-a") == 0) {
        add_to_archive(archive_file, files_dirs, num_files_dirs);
    } else if (strcmp(flag, "-x") == 0) {
        extract_archive(archive_file, files_dirs, num_files_dirs);
    } else if (strcmp(flag, "-j") == 0) {
        compress_and_create_archive(archive_file, files_dirs, num_files_dirs);
    } else if (strcmp(flag, "-d") == 0) {
        delete_from_archive(archive_file, files_dirs, num_files_dirs);
    } else if (strcmp(flag, "-m") == 0) {
        print_metadata(archive_file);
    } else if (strcmp(flag, "-q") == 0) {
        query_archive(archive_file, files_dirs, num_files_dirs);
    } else if (strcmp(flag, "-p") == 0) {
        print_hierarchy(archive_file);
    } else {
        fprintf(stderr, "Invalid flag: %s\n", flag);
        return EXIT_FAILURE;
    }

    return 0;
}
