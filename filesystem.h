
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

typedef struct {
    char *path;
    char *content;
} FileEntry;

typedef struct {
    FileEntry *entries;
    size_t     count;
} FileSystem;

/* загрузка / сохранение / освобождение */
int  fs_load (const char *fname, FileSystem *fs);
int  fs_save (const char *fname, const FileSystem *fs);
void fs_free (FileSystem *fs);

/* CRUD */
int  fs_select(const FileSystem *fs, const char *path, char **out_content);
int  fs_insert(FileSystem *fs, const char *path);
int  fs_update(FileSystem *fs, const char *path, const char *new_content);
int  fs_delete(FileSystem *fs, const char *path);

/* новые функции */
int    fs_crypto (FileSystem *fs, const char *path, const char *key);
int    fs_decrypto(FileSystem *fs, const char *path, const char *key);
size_t fs_count  (const FileSystem *fs);
int    fs_rename (FileSystem *fs, const char *old_path, const char *new_path);

#endif /* FILESYSTEM_H */
