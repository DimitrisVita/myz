# MYZ ARCHIVER

## Description

This project simulates the creation, extraction, and management of archive files using a custom format. The archive can contain files and directories, and supports compression using gzip.

## Design Choices
1. The `filter_paths` function is designed to remove redundant paths from the list of files and directories. This ensures that the archive does not contain duplicate or unnecessary entries, which optimizes the storage and retrieval process.

## Execution Instructions

1. **Compile the project:**
    ```sh
    make all
    ```

2. **Create an archive:**
    ```sh
    ./myz -c <archive-file> <list-of-files/dirs>
    ```

3. **Extract an archive:**
    ```sh
    ./myz -x <archive-file> [list-of-files/dirs]
    ```

4. **Print archive metadata:**
    ```sh
    ./myz -m <archive-file>
    ```

5. **Query files in the archive:**
    ```sh
    ./myz -q <archive-file> <list-of-files/dirs>
    ```

6. **Print archive hierarchy:**
    ```sh
    ./myz -p <archive-file>
    ```

7. **Append files to an archive:**
    ```sh
    ./myz -a <archive-file> <list-of-files/dirs>
    ```

8. **Delete files from an archive:**
    ```sh
    ./myz -d <archive-file> <list-of-files/dirs>
    ```

## Files

- `main.c`: Entry point of the application, parses command line arguments and calls appropriate functions.
- `utils.c`: Utility functions for argument parsing and path filtering.
- `myz.c`: Core functions for creating, extracting, appending, and deleting archives.
- `ADTList.c`: Implementation of a generic linked list.
- `common.h`: Common definitions and structures.
- `utils.h`: Declarations for utility functions.
- `myz.h`: Declarations for core archive functions.
- `ADTList.h`: Declarations for the linked list implementation.
- `Makefile`: Build script for compiling the project.

## Functions

### utils.c

- `print_usage()`: Prints the usage information.

- `filter_paths(char **fileList, int *numFiles)`: Filters redundant paths.

- `parse_arguments(int argc, char *argv[], CommandLineArgs *args)`: Parses command line arguments.

### myz.c

- `create_archive(char *archiveFile, char **fileList, bool gzip)`: Creates an archive.

- `extract_archive(char *archiveFile, char **fileList)`: Extracts files from an archive.

- `append_archive(char *archiveFile, char **fileList, bool gzip)`: Appends files to an archive.

- `delete_archive(char *archiveFile, char **fileList)`: Deletes files from an archive.

- `print_metadata(char *archiveFile)`: Prints the metadata of the archive.

- `query_archive(char *archiveFile, char **fileList)`: Queries files in the archive.

- `print_hierarchy(char *archiveFile)`: Prints the hierarchy of the archive.

### ADTList.c

- `list_create(DestroyFunc destroy_value)`: Creates a new list.

- `list_set_destroy_value(List list, DestroyFunc destroy_value)`: Sets the destroy function for the list.

- `list_destroy(List list)`: Destroys the list and frees all allocated memory.

- `list_insert_after(List list, ListNode node, void* value)`: Inserts a new node after the specified node.

- `list_remove_after(List list, ListNode node)`: Removes the node after the specified node.

- `list_find_value(List list, void* value, int (*compare)(void*, void*))`: Finds and returns the value in the list that matches the given value.

- `list_size(List list)`: Returns the size of the list.

- `list_first(List list)`: Returns the first node in the list.

- `list_next(ListNode node)`: Returns the next node in the list.

- `list_last(List list)`: Returns the last node in the list.

- `list_value(ListNode node)`: Returns the value of the given node.

- `list_find(List list, void* value, int (*compare)(void*, void*))`: Finds and returns the node in the list that matches the given value.
