#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PIPE_CMDS 10

int process_arglist(int count, char **arglist);
int prepare(void);
int finalize(void);

/**
 * @file myshell.c
 * @brief Minimal shell core demonstrating fork/execvp pipelines and redirection.
 *
 * The implementation focuses on low-level process management. It forks once per
 * command, wires pipelines with pipe()/dup2(), and uses execvp() to overlay
 * child processes. SIGCHLD is handled to prevent zombie processes.
 */

/**
 * @brief Handle SIGCHLD and reap children to prevent zombies.
 *
 * The handler loops with waitpid(WNOHANG) to reap any terminated child
 * processes. Errors other than ECHILD are treated as fatal.
 */
static void handle_signal_child(int signum)
{
    (void)signum;
    int old_errno = errno;
    while (1) {
        int child_status = 0;
        pid_t child_pid = waitpid(-1, &child_status, WNOHANG);
        if (child_pid > 0) {
            continue;
        }
        if (child_pid == 0 || errno == ECHILD) {
            break;
        }

        fprintf(stderr, "waitpid error in SIGCHLD handler: %s\n", strerror(errno));
        exit(1);
    }
    errno = old_errno;
}

/**
 * @brief Install signal handling needed by the shell.
 *
 * - Ignore SIGINT in the shell so Ctrl+C only terminates children.
 * - Handle SIGCHLD to reap children asynchronously.
 *
 * @return 0 on success, non-zero on failure.
 */
int prepare(void)
{
    struct sigaction ignore_int;
    memset(&ignore_int, 0, sizeof(ignore_int));
    ignore_int.sa_handler = SIG_IGN;
    ignore_int.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &ignore_int, NULL) == -1) {
        fprintf(stderr, "sigaction(SIGINT) failed: %s\n", strerror(errno));
        return 1;
    }

    struct sigaction reap_children;
    memset(&reap_children, 0, sizeof(reap_children));
    reap_children.sa_handler = handle_signal_child;
    reap_children.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &reap_children, NULL) == -1) {
        fprintf(stderr, "sigaction(SIGCHLD) failed: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

static void close_pipe_set(int pipes[][2], int pipe_count)
{
    for (int i = 0; i < pipe_count; ++i) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}

/**
 * @brief Execute the parsed command list with support for pipelines,
 * background execution, and simple redirection.
 *
 * The function relies on fork/execvp for process creation and uses pipes to
 * connect multiple processes in a pipeline. Foreground children inherit the
 * default SIGINT disposition while background ones ignore it.
 *
 * @param count    Number of arguments (excluding terminating NULL).
 * @param arglist  Parsed command line terminated by NULL.
 *
 * @return 1 on success in the parent shell, 0 on failure.
 */
int process_arglist(int count, char **arglist)
{
    if (count <= 0 || arglist == NULL) {
        return 1;
    }

    int background = 0;
    if (count >= 1 && strcmp(arglist[count - 1], "&") == 0) {
        background = 1;
        arglist[count - 1] = NULL;
        count -= 1;
        if (count == 0) {
            return 1;
        }
    }

    int cmd_starts[MAX_PIPE_CMDS + 1] = {0};
    int cmd_count = 1;
    for (int i = 0; i < count; ++i) {
        if (strcmp(arglist[i], "|") == 0) {
            arglist[i] = NULL;
            cmd_starts[cmd_count++] = i + 1;
        }
    }

    if (cmd_count >= 2) {
        if (background) {
            fprintf(stderr, "Background execution is not supported with pipelines\n");
            return 0;
        }
        if (cmd_count > MAX_PIPE_CMDS) {
            fprintf(stderr, "Pipeline too long (max %d commands)\n", MAX_PIPE_CMDS);
            return 0;
        }

        int pipes[MAX_PIPE_CMDS - 1][2];
        for (int i = 0; i < cmd_count - 1; ++i) {
            if (pipe(pipes[i]) == -1) {
                fprintf(stderr, "pipe failed: %s\n", strerror(errno));
                close_pipe_set(pipes, i);
                return 0;
            }
        }

        pid_t child_pids[MAX_PIPE_CMDS] = {0};
        for (int i = 0; i < cmd_count; ++i) {
            pid_t pid = fork();
            if (pid == -1) {
                fprintf(stderr, "fork failed: %s\n", strerror(errno));
                close_pipe_set(pipes, cmd_count - 1);
                return 0;
            }

            if (pid == 0) {
                struct sigaction sa;
                memset(&sa, 0, sizeof(sa));
                sa.sa_handler = SIG_DFL;
                sigaction(SIGINT, &sa, NULL);

                if (i > 0 && dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    fprintf(stderr, "dup2 failed: %s\n", strerror(errno));
                    exit(1);
                }
                if (i < cmd_count - 1 && dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    fprintf(stderr, "dup2 failed: %s\n", strerror(errno));
                    exit(1);
                }

                close_pipe_set(pipes, cmd_count - 1);

                char **cmd_argv = &arglist[cmd_starts[i]];
                if (cmd_argv[0] == NULL) {
                    fprintf(stderr, "Empty command in pipeline\n");
                    exit(1);
                }

                execvp(cmd_argv[0], cmd_argv);
                fprintf(stderr, "execvp failed (%s): %s\n", cmd_argv[0], strerror(errno));
                exit(1);
            }

            child_pids[i] = pid;
        }

        close_pipe_set(pipes, cmd_count - 1);
        for (int i = 0; i < cmd_count; ++i) {
            int status = 0;
            while (waitpid(child_pids[i], &status, 0) == -1) {
                if (errno == EINTR) {
                    continue;
                }
                if (errno == ECHILD) {
                    break;
                }
                fprintf(stderr, "waitpid failed: %s\n", strerror(errno));
                return 0;
            }
        }

        return 1;
    }

    int redirect_in = 0;
    int redirect_out = 0;
    char *redir_file = NULL;
    if (count >= 2 && strcmp(arglist[count - 2], "<") == 0) {
        redirect_in = 1;
        redir_file = arglist[count - 1];
        arglist[count - 2] = NULL;
    } else if (count >= 2 && strcmp(arglist[count - 2], ">") == 0) {
        redirect_out = 1;
        redir_file = arglist[count - 1];
        arglist[count - 2] = NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
        return 0;
    }

    if (pid == 0) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = (background ? SIG_IGN : SIG_DFL);
        sigaction(SIGINT, &sa, NULL);

        if (redirect_in) {
            int fd = open(redir_file, O_RDONLY);
            if (fd == -1) {
                fprintf(stderr, "open for input failed (%s): %s\n", redir_file, strerror(errno));
                exit(1);
            }
            if (dup2(fd, STDIN_FILENO) == -1) {
                fprintf(stderr, "dup2 failed: %s\n", strerror(errno));
                close(fd);
                exit(1);
            }
            close(fd);
        }
        if (redirect_out) {
            int fd = open(redir_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (fd == -1) {
                fprintf(stderr, "open for output failed (%s): %s\n", redir_file, strerror(errno));
                exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                fprintf(stderr, "dup2 failed: %s\n", strerror(errno));
                close(fd);
                exit(1);
            }
            close(fd);
        }

        execvp(arglist[0], arglist);
        fprintf(stderr, "execvp failed (%s): %s\n", arglist[0], strerror(errno));
        exit(1);
    }

    if (!background) {
        int status = 0;
        while (waitpid(pid, &status, 0) == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == ECHILD) {
                break;
            }
            fprintf(stderr, "waitpid failed: %s\n", strerror(errno));
            return 0;
        }
    }

    return 1;
}

/**
 * @brief Restore default signal dispositions before exit.
 *
 * @return 0 after handlers are restored.
 */
int finalize(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
    return 0;
}
