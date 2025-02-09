#ifndef MYZ_H
#define MYZ_H

void create_archive(const char *archive_file, char **files_dirs, int num_files_dirs);
void add_to_archive(const char *archive_file, char **files_dirs, int num_files_dirs);
void extract_archive(const char *archive_file, char **files_dirs, int num_files_dirs);
void compress_and_create_archive(const char *archive_file, char **files_dirs, int num_files_dirs);
void delete_from_archive(const char *archive_file, char **files_dirs, int num_files_dirs);
void print_metadata(const char *archive_file);
void query_archive(const char *archive_file, char **files_dirs, int num_files_dirs);
void print_hierarchy(const char *archive_file);

#endif // MYZ_H
