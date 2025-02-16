#include "utils.h"

void print_usage() {
    printf("Usage: myz {-c|-a|-x|-m|-d|-p|-j|-q} <archive-file> <list-of-files/dirs>\n");
}

char **filter_paths(char **fileList, int *numFiles) {
    int newSize = 0;
    for (int i = 0; i < *numFiles; i++) {
        if (fileList[i] == NULL) continue;
        for (int j = i + 1; j < *numFiles; j++) {
            if (fileList[j] == NULL) continue;
            if (strstr(fileList[j], fileList[i]) == fileList[j]) {
                free(fileList[j]);
                fileList[j] = NULL;
            } else if (strstr(fileList[i], fileList[j]) == fileList[i]) {
                free(fileList[i]);
                fileList[i] = NULL;
                break;
            }
        }
        if (fileList[i] != NULL) newSize++;
    }

    char **newFileList = malloc((newSize + 1) * sizeof(char *));
    int index = 0;
    for (int i = 0; i < *numFiles; i++) {
        if (fileList[i] != NULL) {
            newFileList[index++] = fileList[i];
        }
    }
    newFileList[newSize] = NULL;

    *numFiles = newSize;
    free(fileList);
    return newFileList;
}

int parse_arguments(int argc, char *argv[], CommandLineArgs *args) {
    int opt;
    // Initialize arguments
    *args = (CommandLineArgs){false, false, false, false, false, false, false, false, NULL, NULL, 0};

    if (argc < 3) {
        print_usage();
        return 1;
    }

    // Parse command line arguments
    while ((opt = getopt(argc, argv, "caxmdpjq")) != -1) {
        switch (opt) {
            case 'c':
                args->create = true;
                break;
            case 'a':
                args->append = true;
                break;
            case 'x':
                args->export = true;
                break;
            case 'm':
                args->metadata = true;
                break;
            case 'd':
                args->delete = true;
                break;
            case 'p':
                args->print = true;
                break;
            case 'q':
                args->query = true;
                break;
            case 'j':
                args->gzip = true;
                break;
            default:
                print_usage();
                return 1;
        }
    }

    // Validate -j flag
    if (args->gzip && !(args->create || args->append)) {
        fprintf(stderr, "-j requires -c or -a\n");
        print_usage();
        return 1;
    }

    // Check if the archive file is provided
    if (optind < argc) {
        args->archiveFile = argv[optind];
        optind++;
    } else {
        print_usage();
        return 1;
    }

    // Copy the list of files and directories to the args structure
    args->fileList = malloc((argc - optind + 1) * sizeof(char *));
    for (int i = 0; i < argc - optind; i++) {
        args->fileList[i] = strdup(argv[optind + i]);
    }
    args->fileList[argc - optind] = NULL; // Terminate with NULL
    args->numFiles = argc - optind;

    // Filter paths
    args->fileList = filter_paths(args->fileList, &args->numFiles);

    return 0;
}