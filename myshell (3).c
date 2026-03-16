#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

int process_arglist(int count, char** arglist);
int prepare(void);
int finalize(void);

 /* מטפל בסיום תהליכי ילד כדי למנוע 
 zombies אם יש שגיאה שהיא לא ECHILD מדפיסים הודעה ויוצאים */
static void handle_signal_child(int signum)
{
    int oldErrno = errno;
    while (1) {
        int child_status;
        pid_t child_pid  = waitpid(-1, &child_status, WNOHANG);
        if (child_pid  > 0) {
            /*ילד אחד הסתיים, נמשיך לבדוק אם יש עוד*/
            continue;
        } else if (child_pid  == 0)
         {
              /*אין עוד ילדים שהשתנ*/
            break;
        } else { /* pid == -1 */
            if (errno == ECHILD) /*/אין יותר ילדים עם PID כזה*/
            {
                break;
            } else {
                //const char *msg = "waitpid error in SIGCHLD handler\n";
                fprintf(stderr, "waitpid error in SIGCHLD handler: %s\n", strerror(errno));
               // write(STDERR_FILENO, msg, strlen(msg));
                exit(1); 
            }
        }
    }
    errno = oldErrno;
}

int prepare(void)
{
   
    struct sigaction new_action1;
    memset(&new_action1, 0, sizeof(new_action1));
    new_action1.sa_handler = SIG_IGN; /* Parent (the shell) should not terminate on SIGINT so ignore SIGINT */
    new_action1.sa_flags = SA_RESTART; /* avoid an EINTR “error” in the first place */
    if (sigaction(SIGINT, &new_action1, NULL) == -1) 
    {
        fprintf(stderr, "sigaction(SIGINT) failed: %s\n", strerror(errno));
        return 1;
    }

    /* מתקינים SIGCHLD handler שמונע זומבים */
    struct sigaction new_action2;
    memset(&new_action2, 0, sizeof(new_action2));
    new_action2.sa_handler = handle_signal_child;
    new_action2.sa_flags = SA_RESTART | SA_NOCLDSTOP; /* don't get notified for stopped children  and  avoid an EINTR “error” in the first place*/
    if (sigaction(SIGCHLD, &new_action2, NULL) == -1) {
        fprintf(stderr, "sigaction(SIGCHLD) failed: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

int process_arglist(int count, char **arglist)
{
    if (count <= 0 || arglist == NULL) return 1; /*not an error, nothing to do */

    
    int background = 0;
    if (count >= 1 && strcmp(arglist[count-1], "&") == 0) /* Detect background '&' (must be last word if present) */
     {
        background = 1;
        arglist[count-1] = NULL; /* remove it from argv passed to exec */
        count -= 1;
        if (count == 0) {return 1; }/* lone '&' so nothing to run */
    }

    int cmd_starts[11]; /* up to 10 commands + sentinel */
    int cmd_count = 1;  /* start with first command at index 0 */
    cmd_starts[0] = 0;
    for (int i = 0; i < count; ++i) {
        if (strcmp(arglist[i], "|") == 0) { /* Detect pipes '|' and split arglist in-place into commands (pointers to words)*/
            arglist[i] = NULL; /* replace '|' tokens by NULL and store pointers to the first word of each command */
            cmd_starts[cmd_count++] = i + 1;
        }
    }

    /* Case A: pipeline (cmd_count >= 2) */
    if (cmd_count >= 2) {
    
        /* limit check allowed up to 10 commands (1..10) */
        if (cmd_count > 10) {
            fprintf(stderr, "Pipeline too long (max 10 commands)\n");
            return 1;
        }

        /* create pipes: need cmd_count - 1 pipes */
        int pipes[9][2];
        for (int i = 0; i < cmd_count - 1; ++i) {
            if (pipe(pipes[i]) == -1) {
                fprintf(stderr, "pipe failed: %s\n", strerror(errno));
                /* close only pipes created so far */
                for (int k = 0; k < i; k++) {
                    close(pipes[k][0]);
                    close(pipes[k][1]);
                }
                return 0;/*In the original (shell/parent) process If process_arglist() encounters anerror, it should print an error message and return 0*/
            }
        }
        pid_t pidsOfChildren[10];
        for (int i = 0; i < cmd_count; ++i) {
            pid_t pid = fork();
            if (pid == -1) {
                fprintf(stderr, "fork failed: %s\n", strerror(errno));
                /* close pipes */
                for (int j = 0; j < cmd_count - 1; ++j) {
                    close(pipes[j][0]); close(pipes[j][1]);
                }
                return 0;/*In the original (shell/parent) process If process_arglist() encounters anerror, it should print an error message and return 0*/

            }

            if (pid == 0)   /* Child process */
            {
                struct sigaction sa;
                memset(&sa, 0, sizeof(sa));
                sa.sa_handler = SIG_DFL; /* default, child should terminate on SIGINT */
                sigaction(SIGINT, &sa, NULL);

                /* Connect pipes: if not first command, set stdin to read end of previous pipe */
                if (i > 0) {
                    if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                        fprintf(stderr, "dup2 failed: %s\n", strerror(errno));
                        exit(1);
                    }
                }
                /* If not last command, set stdout to write end of current pipe */
                if (i < cmd_count - 1) {
                    if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                        fprintf(stderr, "dup2 failed: %s\n", strerror(errno));
                        exit(1);
                    }
                }

                /* Close all pipe fds in child */
                for (int j = 0; j < cmd_count - 1; ++j) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }

                /* Execute command: argv is &arglist[cmd_starts[i]] */
                char **cmd_argv = &arglist[cmd_starts[i]];
                if (cmd_argv[0] == NULL) {
                    fprintf(stderr, "Empty command in pipeline\n");
                    exit(1);
                }
                execvp(cmd_argv[0], cmd_argv);
                /* If execvp returns, an error occurred */
                fprintf(stderr, " failed invalid (%s): %s\n", cmd_argv[0], strerror(errno));
                exit(1);
            } 
            else 
            {
                /* Parent */
                pidsOfChildren[i] = pid;
            }
        }

        /* Parent closes all pipe fds */
        for (int j = 0; j < cmd_count - 1; ++j) {
            close(pipes[j][0]);
            close(pipes[j][1]);
        }
        if (!background) /*per assumptions in pipeline background=0 so we always go into the if section and Wait for all children */
         {
            for (int i = 0; i < cmd_count; ++i) {
                int status;
                while (1) {
                    pid_t curr_child = waitpid(pidsOfChildren[i], &status, 0);
                    if (curr_child== -1) {
                        if (errno == EINTR) continue; /* לא באמת שגיאה, הקריאה ל-waitpid הופסקה בגלל אות שהגיע לא אמור לקרות */
                        if (errno == ECHILD) break; /*אין יותר ילדים עם PID כזה*/
                        fprintf(stderr, "waitpid failed: %s\n", strerror(errno));
                         return 0;/*In the original (shell/parent) process If process_arglist() encounters anerror, it should print an error message and return 0*/
                     

                    }
                    break; /*הילד הסתיים */
                }
            }
        } 
        return 1; 
    }

    /* Case B: single command (no pipes) - check for redirection '<' or '>' */
    int redirect_in = 0, redirect_out = 0;
    char *redir_file = NULL;
    if (count >= 2 && strcmp(arglist[count-2], "<") == 0) /* According to the assumptions, if '<' or '>' appear they are the last two words */
    {
        redirect_in = 1;
        redir_file = arglist[count-1];
        arglist[count-2] = NULL; /* מוחקים < כדי שלא נפרש את הסימן < כארגומנט. */
    } 
    else if (count >= 2 && strcmp(arglist[count-2], ">") == 0)/* According to the assumptions, if '<' or '>' appear they are the last two words */
     {
        redirect_out = 1;
        redir_file = arglist[count-1];
        arglist[count-2] = NULL;/* מוחקים < כדי שלא נפרש את הסימן < כארגומנט. */
    }

    pid_t pid = fork();/* Fork exactly once for the single command */
    if (pid == -1) {
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
        return 0;/*In the original (shell/parent) process If process_arglist() encounters anerror, it should print an error message and return 0*/
    }

    if (pid == 0) {
        /* Child */
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = (background ? SIG_IGN : SIG_DFL); /*/אם זה Foreground אז זה מוגדר להיות ברירת מחדל (כלומר הילד יסתיים) אחרת מתעלמים*/
        sigaction(SIGINT, &sa, NULL);

        /* Redirect input/output if requested.We forked before opening the file. */
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
            int fd = open(redir_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);/*For the open() call, use a mode of 0600 (S_IRUSR | S_IWUSR)*/
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
        /* If execvp returns, print error and exit child */
        fprintf(stderr, " failed invalid (%s): %s\n", arglist[0], strerror(errno));
        exit(1);
    } 
    else 
    {
        /* Parent */
        if (!background)/*אם זה לא פקודה שרצה ברקע אז ההורה מחכה לילד שיסיים*/
         {
            int status;
            while (1) {
                pid_t curr_child = waitpid(pid, &status, 0);
                if (curr_child == -1) 
                {
                    if (errno == EINTR) continue; /* לא באמת שגיאה, הקריאה ל-waitpid הופסקה בגלל אות שהגיע לא אמור לקרות */
                    if (errno == ECHILD) break; /*אין יותר ילדים עם PID כזה*/
                    fprintf(stderr, "waitpid failed: %s\n", strerror(errno));
                    return 0;/*In the original (shell/parent) process If process_arglist() encounters anerror, it should print an error message and return 0*/
                }
                break; 
            }
        } else {
            /*זה לא עושה כלום רק כדי להדגיש שאם זה תהליך שרץ ברקע ההורה לא מחכה */
        }
    }

    return 1;
}

int finalize(void)
{
    /* Restore default signal */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
    return 0;
}
