/**
 * @file shell_main.c
 * @brief Minimal driver for myshell.c to demonstrate command execution.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int prepare(void);
int process_arglist(int count, char **arglist);
int finalize(void);

static int parse_line(char *line, char ***out_args)
{
    *out_args = NULL;
    int capacity = 16;
    int count = 0;
    char **args = malloc((size_t)capacity * sizeof(char *));
    if (!args) {
        return -1;
    }

    char *token = strtok(line, " \t\n");
    while (token != NULL) {
        if (count >= capacity - 1) {
            capacity *= 2;
            char **tmp = realloc(args, (size_t)capacity * sizeof(char *));
            if (!tmp) {
                free(args);
                *out_args = NULL;
                return -1;
            }
            args = tmp;
        }
        args[count++] = token;
        token = strtok(NULL, " \t\n");
    }

    args[count] = NULL;
    *out_args = args;
    return count;
}

int main(void)
{
    if (prepare() != 0) {
        fprintf(stderr, "Failed to initialize shell\n");
        return EXIT_FAILURE;
    }

    char *line = NULL;
    size_t len = 0;
    while (1) {
        fputs("myshell> ", stdout);
        fflush(stdout);
        ssize_t read = getline(&line, &len, stdin);
        if (read == -1) {
            break;
        }
        if (read == 0 || strcmp(line, "\n") == 0) {
            continue;
        }

        char **args = NULL;
        int argc = parse_line(line, &args);
        if (argc < 0) {
            fprintf(stderr, "Failed to parse command line (allocation error)\n");
            continue;
        }
        if (argc == 0) {
            free(args);
            continue;
        }

        int ok = process_arglist(argc, args);
        if (!ok) {
            fprintf(stderr, "process_arglist reported an error\n");
        }
        free(args);
    }

    free(line);
    finalize();
    return EXIT_SUCCESS;
}
