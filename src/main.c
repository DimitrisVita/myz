#include "common.h"
#include "utils.h"
#include "myz.h"

int main(int argc, char *argv[]) {
    CommandLineArgs args;

    if (parse_arguments(argc, argv, &args) != 0)
        return 1;

    // Call the appropriate function based on the command line arguments
    if (args.create && args.filePath) {
        create_archive(args.archiveFile, args.filePath, args.gzip);
    } else if (args.append && args.filePath) {
        append_archive(args.archiveFile, args.filePath, args.gzip);
    } else if (args.export) {
        extract_archive(args.archiveFile, args.filePath);
    } else if (args.delete && args.filePath) {
        delete_archive(args.archiveFile, args.filePath);
    } else if (args.metadata && !args.filePath) {
        print_metadata(args.archiveFile);
    } else if (args.query && args.filePath) {
        query_archive(args.archiveFile, args.filePath);
    } else if (args.print && !args.filePath) {
        print_hierarchy(args.archiveFile);
    } else {
        print_usage();
        return 1;
    }

    return 0;
}
