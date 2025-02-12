#include "utils.h"

void print_usage() {
    printf("Usage: myz {-c|-a|-x|-m|-d|-p|-j|-q} <archive-file> <list-of-files/dirs>\n");
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
    args->fileList = &argv[optind];
    args->numFiles = argc - optind;

    // SOS AN EXW MEMORY LEAK EDW THA EINAI EPEIDH DEN KANW MALLOC


    return 0;
}
