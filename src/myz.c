#include "myz.h"
#include "common.h"
#include "utils.h"
#include "ADTList.h" // Include ADTList header
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <zlib.h>
#include <limits.h> // Include this header for PATH_MAX
#include <sys/wait.h> // Include this header for waitpid
#include <stdint.h> // Include this header for uint32_t

#define PATH_MAX 4096 // Define PATH_MAX if not defined

typedef struct {
    char *path;
    struct stat metadata;
    off_t data_offset;
    size_t data_size;
    char *link_target;
} FileEntry;

typedef struct Node {
    FileEntry entry;
    struct Node *next;
} Node;

typedef struct {
    char magic[4];          // Magic number (e.g., "MYZ\0")
    uint32_t num_entries;   // Number of entries in the archive
    off_t metadata_offset;  // Offset to metadata section
    uint8_t flags;          // Flags (e.g., compression enabled)
} MyzHeader;

void add_file_entry(Node **head, const char *path, struct stat *metadata, off_t data_offset, size_t data_size, const char *link_target) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->entry.path = strdup(path);
    new_node->entry.metadata = *metadata;
    new_node->entry.data_offset = data_offset;
    new_node->entry.data_size = data_size;
    new_node->entry.link_target = link_target ? strdup(link_target) : NULL;
    new_node->next = *head;
    *head = new_node;
}

void write_metadata(int fd, Node *head) {
    Node *current = head;
    while (current) {
        write(fd, current->entry.path, strlen(current->entry.path) + 1); // Write path as string

        // Serialize metadata in a platform-independent way
        uint32_t mode = (uint32_t)current->entry.metadata.st_mode;
        uint64_t size = (uint64_t)current->entry.metadata.st_size;
        uint64_t data_offset = (uint64_t)current->entry.data_offset;
        uint64_t data_size = (uint64_t)current->entry.data_size;

        write(fd, &mode, sizeof(uint32_t));
        write(fd, &size, sizeof(uint64_t));
        write(fd, &data_offset, sizeof(uint64_t));
        write(fd, &data_size, sizeof(uint64_t));

        // Write other metadata fields as needed
        current = current->next;
    }
}

void free_file_entries(Node *head) {
    Node *current = head;
    while (current) {
        free(current->entry.path);
        if (current->entry.link_target) {
            free(current->entry.link_target);
        }
        Node *temp = current;
        current = current->next;
        free(temp);
    }
}

void process_directory(const char *dir_path, List *fileList, int fd, bool gzip) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat metadata;
        if (lstat(full_path, &metadata) == -1) continue;

        if (S_ISDIR(metadata.st_mode)) {
            process_directory(full_path, fileList, fd, gzip);
        } else if (S_ISREG(metadata.st_mode)) {
            int file_fd = open(full_path, O_RDONLY);
            if (file_fd == -1) continue;

            off_t data_offset = lseek(fd, 0, SEEK_CUR);
            char buffer[4096];
            ssize_t bytes_read;
            if (gzip) {
                int pipefd[2];
                if (pipe(pipefd) == -1) {
                    perror("Failed to create pipe");
                    close(file_fd);
                    return;
                }

                pid_t pid = fork();
                if (pid == -1) {
                    perror("Failed to fork");
                    close(file_fd);
                    return;
                }

                if (pid == 0) { // Child process
                    close(pipefd[1]); // Close write end of the pipe
                    dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to read end of the pipe
                    dup2(fd, STDOUT_FILENO); // Redirect stdout to the archive file
                    execlp("gzip", "gzip", "-c", NULL); // Execute gzip
                    perror("Failed to exec gzip");
                    exit(EXIT_FAILURE);
                } else { // Parent process
                    close(pipefd[0]); // Close read end of the pipe
                    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
                        write(pipefd[1], buffer, bytes_read); // Write to the pipe
                    }
                    close(pipefd[1]); // Close write end of the pipe
                    waitpid(pid, NULL, 0); // Wait for the child process to finish
                }
            } else {
                while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
                    write(fd, buffer, bytes_read);
                }
            }
            close(file_fd);

            size_t data_size = lseek(fd, 0, SEEK_CUR) - data_offset;
            list_insert_next(fileList, list_last(fileList), strdup(full_path));
        } else if (S_ISLNK(metadata.st_mode)) {
            char link_target[PATH_MAX];
            ssize_t len = readlink(full_path, link_target, sizeof(link_target) - 1);
            if (len != -1) {
                link_target[len] = '\0';
                list_insert_next(fileList, list_last(fileList), strdup(full_path));
            }
        }
    }
    closedir(dir);
}

void create_archive(const char *archiveFile, List *fileList, bool gzip) {
    int fd = open(archiveFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Failed to create archive file");
        return;
    }

    // Write header (placeholder)
    MyzHeader header = { .magic = "MYZ", .num_entries = 0, .metadata_offset = 0, .flags = gzip ? 1 : 0 };
    write(fd, &header, sizeof(MyzHeader));

    ListNode *node = list_first(fileList);
    while (node != NULL) {
        const char *filePath = list_node_value(fileList, node);
        struct stat metadata;
        if (lstat(filePath, &metadata) == -1) {
            perror("Failed to stat input file/directory");
            close(fd);
            return;
        }

        if (S_ISDIR(metadata.st_mode)) {
            process_directory(filePath, fileList, fd, gzip);
        } else if (S_ISREG(metadata.st_mode)) {
            int file_fd = open(filePath, O_RDONLY);
            if (file_fd == -1) {
                perror("Failed to open input file");
                close(fd);
                return;
            }

            off_t data_offset = lseek(fd, 0, SEEK_CUR);
            char buffer[4096];
            ssize_t bytes_read;
            if (gzip) {
                int pipefd[2];
                if (pipe(pipefd) == -1) {
                    perror("Failed to create pipe");
                    close(file_fd);
                    return;
                }

                pid_t pid = fork();
                if (pid == -1) {
                    perror("Failed to fork");
                    close(file_fd);
                    return;
                }

                if (pid == 0) { // Child process
                    close(pipefd[1]); // Close write end of the pipe
                    dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to read end of the pipe
                    dup2(fd, STDOUT_FILENO); // Redirect stdout to the archive file
                    execlp("gzip", "gzip", "-c", NULL); // Execute gzip
                    perror("Failed to exec gzip");
                    exit(EXIT_FAILURE);
                } else { // Parent process
                    close(pipefd[0]); // Close read end of the pipe
                    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
                        write(pipefd[1], buffer, bytes_read); // Write to the pipe
                    }
                    close(pipefd[1]); // Close write end of the pipe
                    waitpid(pid, NULL, 0); // Wait for the child process to finish
                }
            } else {
                while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
                    write(fd, buffer, bytes_read);
                }
            }
            close(file_fd);

            size_t data_size = lseek(fd, 0, SEEK_CUR) - data_offset;
            list_insert_next(fileList, list_last(fileList), strdup(filePath));
        }

        node = list_next(fileList, node);
    }

    // Update header with metadata offset and number of entries
    header.metadata_offset = lseek(fd, 0, SEEK_CUR);
    header.num_entries = list_size(fileList);
    lseek(fd, 0, SEEK_SET);
    write(fd, &header, sizeof(MyzHeader));

    // Write metadata
    lseek(fd, header.metadata_offset, SEEK_SET);
    // write_metadata(fd, fileList); // Implement write_metadata to handle ADTList

    close(fd);
}

