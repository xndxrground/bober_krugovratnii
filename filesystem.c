
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"

static int find_index(const FileSystem *fs, const char *path) {
    for (size_t i = 0; i < fs->count; ++i)
        if (strcmp(fs->entries[i].path, path) == 0)
            return (int)i;
    return -1;
}

/* ——— базовые операции загрузки и сохранения ——— */
int fs_load(const char *fname, FileSystem *fs) {
    FILE *f = fopen(fname, "r");
    if (!f) {                        /* нет файла — пустая ФС */
        fs->entries = NULL;
        fs->count = 0;
        return 0;
    }

    char *line = NULL;
    size_t nline = 0;
    ssize_t n;
    char *cur_path = NULL, *cur_buf = NULL;
    size_t cur_cap = 0, cur_len = 0;

    while ((n = getline(&line, &nline, f)) != -1) {
        if (line[0] == '/') {                    /* новая запись */
            if (cur_path) {                      /* записываем предыдущую */
                fs_insert(fs, cur_path);
                fs_update(fs, cur_path, cur_buf ? cur_buf : "");
            }
            free(cur_path);
            cur_path = strdup(line);
            size_t L = strlen(cur_path);
            if (L && (cur_path[L-1] == '\n' || cur_path[L-1] == '\r'))
                cur_path[L-1] = '\0';

            free(cur_buf);
            cur_buf = NULL; cur_cap = cur_len = 0;
        } else {                                 /* контент */
            size_t l = (size_t)n;
            if (cur_len + l + 1 > cur_cap) {
                cur_cap = (cur_cap ? cur_cap * 2 : 4096) + l;
                cur_buf = realloc(cur_buf, cur_cap);
            }
            memcpy(cur_buf + cur_len, line, l);
            cur_len += l;
            cur_buf[cur_len] = '\0';
        }
    }
    if (cur_path) {                              /* последний файл */
        fs_insert(fs, cur_path);
        fs_update(fs, cur_path, cur_buf ? cur_buf : "");
    }

    free(cur_path);
    free(cur_buf);
    free(line);
    fclose(f);
    return 0;
}

int fs_save(const char *fname, const FileSystem *fs) {
    FILE *f = fopen(fname, "w");
    if (!f) return -1;
    for (size_t i = 0; i < fs->count; ++i) {
        fprintf(f, "%s\n", fs->entries[i].path);
        fputs(fs->entries[i].content, f);
        size_t L = strlen(fs->entries[i].content);
        if (L == 0 || fs->entries[i].content[L-1] != '\n')
            fputc('\n', f);
    }
    fclose(f);
    return 0;
}

void fs_free(FileSystem *fs) {
    for (size_t i = 0; i < fs->count; ++i) {
        free(fs->entries[i].path);
        free(fs->entries[i].content);
    }
    free(fs->entries);
    fs->entries = NULL;
    fs->count = 0;
}

/* CRUD --------------------------------------------------------------*/
int fs_select(const FileSystem *fs, const char *path, char **out_content) {
    int idx = find_index(fs, path);
    if (idx < 0) return -1;
    *out_content = strdup(fs->entries[idx].content);
    return 0;
}

int fs_insert(FileSystem *fs, const char *path) {
    if (find_index(fs, path) >= 0) return -1;    /* уже есть */

    FileEntry *tmp = realloc(fs->entries, (fs->count + 1) * sizeof *tmp);
    if (!tmp) return -1;
    fs->entries = tmp;

    fs->entries[fs->count].path = strdup(path);
    fs->entries[fs->count].content = strdup("");
    fs->count++;
    return 0;
}

int fs_update(FileSystem *fs, const char *path, const char *new_content) {
    int idx = find_index(fs, path);
    if (idx < 0) return -1;
    free(fs->entries[idx].content);
    fs->entries[idx].content = strdup(new_content);
    return 0;
}

int fs_delete(FileSystem *fs, const char *path) {
    int idx = find_index(fs, path);
    if (idx < 0) return -1;
    free(fs->entries[idx].path);
    free(fs->entries[idx].content);
    memmove(&fs->entries[idx], &fs->entries[idx + 1],
            (fs->count - idx - 1) * sizeof(FileEntry));
    fs->count--;
    return 0;
}

/* ——— дополнительные функции ——— */

/* XOR helper */
static char *xor_buffer(const char *src, const char *key) {
    size_t n = strlen(src), klen = strlen(key);
    if (klen == 0) return NULL;
    char *dst = malloc(n + 1);
    if (!dst) return NULL;
    for (size_t i = 0; i < n; ++i)
        dst[i] = src[i] ^ key[i % klen];
    dst[n] = '\0';
    return dst;
}

int fs_crypto(FileSystem *fs, const char *path, const char *key) {
    int idx = find_index(fs, path);
    if (idx < 0) return -1;
    char *enc = xor_buffer(fs->entries[idx].content, key);
    if (!enc) return -1;
    free(fs->entries[idx].content);
    fs->entries[idx].content = enc;
    return 0;
}

int fs_decrypto(FileSystem *fs, const char *path, const char *key) {
    /* XOR тем же ключом -> исходный текст */
    return fs_crypto(fs, path, key);
}

size_t fs_count(const FileSystem *fs) {
    return fs->count;
}

int fs_rename(FileSystem *fs, const char *old_path, const char *new_path) {
    if (find_index(fs, new_path) >= 0)    /* уже есть файл с новым именем */
        return -2;
    int idx = find_index(fs, old_path);
    if (idx < 0) return -1;
    free(fs->entries[idx].path);
    fs->entries[idx].path = strdup(new_path);
    return 0;
}
