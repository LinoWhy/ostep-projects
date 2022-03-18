#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_LEN 256

int main(int argc, char const *argv[]) {
    char buf[BUF_LEN];

    if (argc < 2) {
        // printf("Uasge: wcat <filename>");
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        FILE *fp = fopen(argv[i], "r");
        if (fp == NULL) {
            printf("wcat: cannot open file\n");
            exit(1);
        }

        /* read & print for each line */
        while (NULL != fgets(buf, BUF_LEN, fp)) {
            printf("%s", buf);
        }

        fclose(fp);
    }

    return 0;
}
