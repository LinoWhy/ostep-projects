#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_LEN 4096 /* line limit */

int main(int argc, char const *argv[]) {
    FILE *input = stdin;
    FILE *output = stdout;
    struct stat s1, s2;
    char *lineptr = NULL;
    size_t n = 0;
    unsigned int count = 0;

    /* linked list is better */
    char *stack[BUF_LEN];
    unsigned int num[BUF_LEN];

    switch (argc) {
        case 1:
            /* stdin is redirected & argc=1 for test 7 ("<")  */
            break;
        case 2:
            input = fopen(argv[1], "r");
            if (input == NULL) {
                fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
                exit(1);
            }
            break;
        case 3:
            input = fopen(argv[1], "r");
            if (input == NULL) {
                fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
                exit(1);
            }

            /* use "r+" to avoid changing the same file */
            output = fopen(argv[2], "r+");
            if (output == NULL) {
                fprintf(stderr, "reverse: cannot open file '%s'\n", argv[2]);
                exit(1);
            }

            /* check if two files having same inode */
            stat(argv[1], &s1);
            stat(argv[2], &s2);
            if (s1.st_ino == s2.st_ino) {
                fprintf(stderr, "reverse: input and output file must differ\n");
                exit(1);
            }
            break;
        default:
            fprintf(stderr, "usage: reverse <input> <output>\n");
            exit(1);
    }

    /* read each line & store in stack */
    for (int i = 0; i < BUF_LEN; i++) {
        int rc = getline(&lineptr, &n, input);
        if (rc > 0) {
            stack[i] = lineptr;
            num[i] = rc;
            count++;

            lineptr = NULL;
            n = 0;
        } else {
            free(lineptr);
            break;
        }
    }

    /* write each line to output file */
    for (int i = count; i >= 0; i--) {
        fwrite(stack[i], 1, num[i], output);
        free(stack[i]);
    }

    fclose(input);
    fclose(output);
    return 0;
}
