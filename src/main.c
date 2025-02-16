#include "common.h"
#include "utils.h"
#include "myz.h"

int main(int argc, char *argv[]) {
    CommandLineArgs args;

    if (parse_arguments(argc, argv, &args) != 0)
        return 1;

    // Call the appropriate function based on the command line arguments
    if (args.create && args.fileList) {
        create_archive(args.archiveFile, args.fileList, args.gzip);
    } else if (args.export) {
        extract_archive(args.archiveFile, args.fileList);
    } else if (args.metadata && !args.fileList) {
        print_metadata(args.archiveFile);
    } else {
        print_usage();
        return 1;
    }

    //  else if (args.append && args.fileList) {
    //     append_archive(args.archiveFile, args.fileList, args.gzip);
    // } else if (args.export) {
    //     extract_archive(args.archiveFile, args.fileList[0]);
    // } else if (args.delete && args.fileList) {
    //     delete_archive(args.archiveFile, args.fileList[0]);
    // } else if (args.metadata && !args.fileList) {
    //     print_metadata(args.archiveFile);
    // } else if (args.query && args.fileList) {
    //     query_archive(args.archiveFile, args.fileList[0]);
    // } else if (args.print && !args.fileList) {
    //     print_hierarchy(args.archiveFile);
    // } else {
    //     print_usage();
    //     return 1;
    // }

    return 0;
}
