
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "filesystem.h"

/* путь к контейнеру ФС */
static const char *FS_PATH = "disk.filesystem";

static void trim_trailing_newline(char *s) {
    size_t n = strlen(s);
    if (n && (s[n-1] == '\n' || s[n-1] == '\r'))
        s[n-1] = '\0';
}

static void print_usage(void) {
    puts("Команды:");
    puts("  SELECT <path>");
    puts("  INSERT <path>");
    puts("  UPDATE <path>");
    puts("       (завершите ввод строкой только с точкой '.')");
    puts("  DELETE <path>");
    puts("  CRYPTO <path> <key>");
    puts("  DECRYPTO <path> <key>");
    puts("  COUNT");
    puts("  RENAME <oldPath> <newPath>");
    puts("  EXIT");
}

int main(void) {
    FileSystem fs = {0};

    if (fs_load(FS_PATH, &fs) != 0) {
        fprintf(stderr, "Не удалось загрузить %s\n", FS_PATH);
        return 1;
    }

    puts("Псевдо-ФС загружена. Введите HELP для подсказки.");

    char line[8192];
    for (;;) {
        printf("> ");
        if (!fgets(line, sizeof line, stdin))
            break;
        trim_trailing_newline(line);

        char *cmd = strtok(line, " ");
        if (!cmd) continue;
        for (char *p = cmd; *p; ++p) *p = toupper(*p);

        if (strcmp(cmd, "HELP") == 0) {
            print_usage();
        } else if (strcmp(cmd, "EXIT") == 0) {
            break;
        } else if (strcmp(cmd, "SELECT") == 0) {
            char *path = strtok(NULL, "");
            if (!path) { puts("Нужен путь"); continue; }
            char *content = NULL;
            if (fs_select(&fs, path, &content) == 0)
                printf("%s\n", content);
            else
                printf("Файл '%s' не найден\n", path);
            free(content);
        } else if (strcmp(cmd, "INSERT") == 0) {
            char *path = strtok(NULL, "");
            if (!path) { puts("Нужен путь"); continue; }
            puts(fs_insert(&fs, path) == 0 ? "Готово" : "Такой файл уже существует");
        } else if (strcmp(cmd, "DELETE") == 0) {
            char *path = strtok(NULL, "");
            if (!path) { puts("Нужен путь"); continue; }
            puts(fs_delete(&fs, path) == 0 ? "Удалено" : "Файл не найден");
        } else if (strcmp(cmd, "UPDATE") == 0) {
            char *path = strtok(NULL, "");
            if (!path) { puts("Нужен путь"); continue; }
            puts("Введите новые данные (строка "." завершает ввод):");
            char *buf = NULL;
            size_t cap = 0, len = 0;
            while (fgets(line, sizeof line, stdin)) {
                if (strcmp(line, ".\n") == 0 || strcmp(line, ".\r\n") == 0)
                    break;
                size_t l = strlen(line);
                if (len + l + 1 > cap) {
                    cap = (cap ? cap * 2 : 4096) + l;
                    buf = realloc(buf, cap);
                }
                memcpy(buf + len, line, l);
                len += l;
            }
            if (buf) buf[len] = '\0';
            puts(fs_update(&fs, path, buf ? buf : "") == 0 ? "Перезаписано" : "Файл не найден");
            free(buf);
        } else if (strcmp(cmd, "COUNT") == 0) {
            printf("%zu\n", fs_count(&fs));
        } else if (strcmp(cmd, "CRYPTO") == 0) {
            char *path = strtok(NULL, " ");
            char *key  = strtok(NULL, "");
            if (!path || !key) { puts("Нужно: CRYPTO <path> <key>"); continue; }
            puts(fs_crypto(&fs, path, key) == 0 ? "Зашифровано" : "Ошибка");
        } else if (strcmp(cmd, "DECRYPTO") == 0) {
            char *path = strtok(NULL, " ");
            char *key  = strtok(NULL, "");
            if (!path || !key) { puts("Нужно: DECRYPTO <path> <key>"); continue; }
            puts(fs_decrypto(&fs, path, key) == 0 ? "Расшифровано" : "Ошибка");
        } else if (strcmp(cmd, "RENAME") == 0) {
            char *oldp = strtok(NULL, " ");
            char *newp = strtok(NULL, "");
            if (!oldp || !newp) { puts("Нужно: RENAME <old> <new>"); continue; }
            int rc = fs_rename(&fs, oldp, newp);
            if (rc == 0)           puts("Переименовано");
            else if (rc == -2)     puts("Файл с таким именем уже есть");
            else                   puts("Файл не найден");
        } else {
            puts("Неизвестная команда или опечатка");
        }
    }

    fs_save(FS_PATH, &fs);
    fs_free(&fs);
    puts("Изменения сохранены. Пока!");
    return 0;
}
