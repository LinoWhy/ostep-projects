#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define BUF_LEN 2
#define INT_LEN 4

void zip(char *bufp) {
    char *c = bufp;
    char *w = bufp;
    unsigned int count = 1;

    while ((*c != '\0')) {
        for (w = c + 1; *c == *w; w++) {
            count++;
        }

        fwrite(&count, INT_LEN, 1, stdout);
        fwrite(c, 1, 1, stdout);

        /* init for next iteration */
        count = 1;
        c = w;
    }
}

int main(int argc, char const *argv[]) {
    char buf[BUF_LEN];
    char *bufp = NULL;
    size_t size = 0;
    unsigned n = 0;

    if (argc < 2) {
        printf("wzip: file1 [file2 ...]\n");
        return 1;
    }

    /* open a memory stream as buffer */
    FILE *tmp = open_memstream(&bufp, &size);
    if (tmp == NULL) {
        printf("wzip: cannot open memory stream\n");
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        /* open each input file */
        FILE *fp = fopen(argv[i], "r");
        if (fp == NULL) {
            printf("wzip: cannot open file\n");
            exit(1);
        }

        /* copy to memory stream */
        while ((n = fread(buf, 1, BUF_LEN - 1, fp)) > 0) {
            /* restrict end of string */
            buf[n] = '\0';
            fwrite(buf, 1, n, tmp);
            fflush(tmp);
        }

        fclose(fp);
    }

    zip(bufp);
    fclose(tmp);
    free(bufp);

    return 0;
}
