#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXARG 256
#define MAXLINE 4096
#define DEBUG 1
#define LOG_D(fmt, ...)                                                \
    if (DEBUG) {                                                       \
        fprintf(flog, "{%d | %s: %d} ", getpid(), __func__, __LINE__); \
        fprintf(flog, fmt, ##__VA_ARGS__);                             \
        fflush(flog);                                                  \
    }

char error_message[30] = "An error has occurred\n";
FILE *flog = NULL;
char path[MAXLINE] = "/bin;";
char *args[MAXARG];
unsigned int argCount = 0;

/* try built-in commands
 * return 0: not built-in commands
 * return 1: is built-in commands & shall continue
 * return -1: is built-in commands & shall break
 */
int do_builtin(char *argv[], unsigned int count) {
    int rc = 0;

    /* check built-in commands */
    if (0 == strcmp(argv[0], "exit")) {
        LOG_D("get command: exit\n");

        /* exit support no argument */
        if (1 == count) {
            rc = -1;
        } else {
            write(STDERR_FILENO, error_message, strlen(error_message));
            rc = 1;
        }
    } else if (0 == strcmp(argv[0], "cd")) {
        LOG_D("get command: cd\n");

        /* cd only support one argument */
        if (2 == count) {
            rc = chdir(argv[1]);
            if (-1 == rc) {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
        } else {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        rc = 1;
    } else if (0 == strcmp(argv[0], "path")) {
        LOG_D("get command: path\n");

        /* overwrite path */
        path[0] = '\0';
        for (int i = 1; i < count; i++) {
            if ('/' != *argv[i]) {
                strcat(path, "./");
            }
            strcat(path, argv[i]);
            strcat(path, ";");
        }

        LOG_D("path: %s\n", path);
        rc = 1;
    } else {
        /* not built-in command */
        rc = 0;
    }
    return rc;
}

/* fork new process for each new command
 * return 1: shall continue
 * return -1: shall break
 */
int do_newcmd(char *argv[], unsigned int count) {
    int rc = -1;
    int fd = STDOUT_FILENO;
    unsigned int isRedirect = 0;
    char buf[MAXLINE];
    char *s = NULL;
    char *pathOrig = strdup(path);
    char *p = pathOrig;

    /* clear buffer */
    buf[0] = '\0';
    /* check access under each path */
    s = strsep(&p, ";");
    while (NULL != p) {
        sprintf(buf, "%s/%s", s, argv[0]);
        LOG_D("buf: %s, p: %s\n", buf, p);

        rc = access(buf, F_OK | X_OK);
        if (0 == rc) {
            /* access returns ok */
            break;
        }
        /* try next path */
        s = strsep(&p, ";");
    }
    free(pathOrig);
    LOG_D("access returns %d\n", rc);
    if (rc < 0) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 1;
    }

    /* check redirection */
    for (int i = 1; i < count; i++) {
        if (0 == strcmp(argv[i], ">")) {
            if (i != count - 2) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                return 1;
            } else {
                /* open the file after '>' */
                fd = open(argv[i + 1], O_CREAT | O_TRUNC);
                if (-1 == fd) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    return 1;
                }
                isRedirect = 1;
                break;
            }
        }
    }

    /* fork child process for the jobs */
    pid_t pid = fork();

    /* child process */
    if (0 == pid) {
        LOG_D("commands: %s\n", buf);

        if (1 == isRedirect) {
            dup2(fd, STDOUT_FILENO);
        }
        rc = execv(buf, argv);
        /* shall never reaches here */
        LOG_D("execv returns %d\n", rc);
        write(STDERR_FILENO, error_message, strlen(error_message));
        return -1;
    }

    /* parent process */
    while (wait(NULL) > 0) {
        // sleep(1);
    }
    return 1;
}

void do_parse(char *lineptr) {
    char *tmp = NULL;

    argCount = 0;
    /* delimate with white space, tab and line feed */
    tmp = strsep(&lineptr, " \t\n");
    LOG_D("tmp: %s, input: %s\n", tmp, lineptr);

    while (NULL != lineptr) {
        /* ignore preceding white spaces or tabs */
        if (0 != *tmp) {
            args[argCount] = tmp;
            argCount++;
        }

        tmp = strsep(&lineptr, " \t\n");
        LOG_D("tmp: %s, input: %s\n", tmp, lineptr);
    }
    /* empty line */
    if (0 == argCount) {
        return;
    }
    /* arg list ends with NULL pointer */
    args[argCount] = NULL;

    for (int i = 0; i < argCount; i++) {
        LOG_D("argv[%d]: %s\n", i, args[i]);
    }
}

int main(int argc, char const *argv[]) {
    int rc = 0;
    unsigned int isbatch = 0;
    FILE *fin = stdin;
    char *lineptr = NULL;
    size_t n = 0;

    /* log file instead of standard stream */
    flog = fopen("./log.txt", "w");
    if (NULL == flog) {
        LOG_D("Error: unable to create log file!\n");
        exit(-1);
    }

    LOG_D("binary: %s\n", argv[0]);

    if (argc > 2) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    /* open batch file if necessary */
    if (argc == 2) {
        LOG_D("arg: %s\n", argv[1]);
        isbatch = 1;
        fin = fopen(argv[1], "r");
        if (NULL == fin) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    }

    while (1) {
        LOG_D("--- NEW LOOP ---\n");

        /* prompt only for interactive mode */
        if (0 == isbatch) {
            printf("wish> ");
        }

        /* get one input line */
        rc = getline(&lineptr, &n, fin);
        if (-1 == rc) {
            /* error or EOF */
            LOG_D("EOF!\n");
            exit(0);
        }

        /* extract command line into arguments */
        do_parse(lineptr);

        /* try built-in commands */
        rc = do_builtin(args, argCount);
        LOG_D("do_builtin() returns %d\n", rc);
        if (1 == rc) {
            continue;
        } else if (-1 == rc) {
            break;
        }

        rc = do_newcmd(args, argCount);
        LOG_D("do_newcmd() returns %d\n", rc);
        if (1 == rc) {
            continue;
        } else if (-1 == rc) {
            break;
        }
    }

    LOG_D("--- END OF EXECUTION ---\n");
    free(lineptr);
    return 0;
}
