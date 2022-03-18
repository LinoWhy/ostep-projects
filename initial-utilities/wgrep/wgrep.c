#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_LEN 256

void grepline(char *lineptr, char const *pattern) {
    if (NULL != strstr(lineptr, pattern)) {
        printf("%s", lineptr);
    }
}

int main(int argc, char const *argv[]) {
    char *lineptr = NULL;
    size_t n = 0;

    if (argc < 2) {
        printf("wgrep: searchterm [file ...]\n");
        return 1;
    }

    if (argc == 2) {
        /* no file specified */
        while (getline(&lineptr, &n, stdin) > 0) {
            grepline(lineptr, argv[1]);
        }
        return 0;
    }

    for (int i = 2; i < argc; i++) {
        FILE *fp = fopen(argv[i], "r");
        if (fp == NULL) {
            printf("wgrep: cannot open file\n");
            exit(1);
        }

        while (getline(&lineptr, &n, fp) > 0) {
            grepline(lineptr, argv[1]);
        }

        fclose(fp);
    }

    free(lineptr);
    return 0;
}
