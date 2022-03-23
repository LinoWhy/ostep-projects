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
char path_origin[8] = "/bin";

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
    int rc = 0;
    char buf[MAXLINE];

    /* check access */
    sprintf(buf, "%s/%s", path_origin, argv[0]);
    LOG_D("commands: %s\n", buf);
    rc = access(buf, F_OK | X_OK);
    LOG_D("access returns %d\n", rc);
    if (rc < 0) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 1;
    }

    /* fork child process for the jobs */
    pid_t pid = fork();

    /* child process */
    if (0 == pid) {
        rc = execv(buf, argv);
        /* shall never reaches here */
        LOG_D("execv returns %d\n", rc);
        /* child process shall end if execute failed */
        return -1;
    }

    /* parent process */
    while (wait(NULL) > 0) {
        // sleep(1);
    }
    return 1;
}

int main(int argc, char const *argv[]) {
    int rc = 0;
    unsigned int isbatch = 0;
    FILE *fin = stdin;
    char *lineptr = NULL;
    char *lineOrg = NULL;
    size_t n = 0;
    char *tmp;
    char *args[MAXARG];
    unsigned int argCount = 0;

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
        argCount = 0;
        lineOrg = lineptr;
        /* delimate with white spac, tab and line feed */
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
        /* arg list ends with NULL pointer */
        args[argCount] = NULL;

        for (int i = 0; i < argCount; i++) {
            LOG_D("argv[%d]: %s\n", i, args[i]);
        }

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
    free(lineOrg);
    return 0;
}
