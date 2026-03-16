/**
 * @file shell.h
 * @brief Public API for mini shell component.
 */

#ifndef SHELL_H
#define SHELL_H

/**
 * @brief Initialize the shell environment and signal handlers.
 *
 * Sets up:
 * - SIGINT handling (ignored in parent, default in children)
 * - SIGCHLD handling (async zombie reaping)
 *
 * Must be called before any command processing.
 *
 * @return 0 on success, non-zero on failure
 */
int prepare(void);

/**
 * @brief Process and execute a parsed command line.
 *
 * Supports:
 * - Simple commands: argv[0] is the command
 * - Pipelines: Commands separated by "|" tokens
 * - I/O redirection: "<" for input, ">" for output
 * - Background jobs: Trailing "&" token
 *
 * @param count Number of arguments (excluding NULL terminator)
 * @param arglist Array of command/argument strings, NULL-terminated
 * @return 1 on success, 0 on failure
 */
int process_arglist(int count, char **arglist);

/**
 * @brief Clean up shell resources and restore default signal handlers.
 *
 * Should be called before program exit.
 *
 * @return 0 on success
 */
int finalize(void);

#endif /* SHELL_H */
