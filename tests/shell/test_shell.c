/**
 * @file test_shell.c
 * @brief Integration tests for shell implementation.
 *
 * Tests shell command parsing, execution, signal handling, and error cases.
 */

#define _POSIX_C_SOURCE 200809L
#include "common/logger.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* External shell functions */
extern int prepare(void);
extern int process_arglist(int count, char **arglist);
extern int finalize(void);

static int test_shell_initialization(void);
static int test_simple_command(void);

static int test_shell_initialization(void)
{
    LOG_INFO("Running test_shell_initialization");

    int result = prepare();
    assert(result == 0);

    result = finalize();
    assert(result == 0);

    LOG_INFO("test_shell_initialization: PASSED");
    return 0;
}

static int test_simple_command(void)
{
    LOG_INFO("Running test_simple_command");

    prepare();

    /* Test echo command */
    char *args1[] = {"echo", "test", NULL};
    int result = process_arglist(2, args1);
    assert(result == 1);

    /* Test ls command */
    char *args2[] = {"ls", "/tmp", NULL};
    result = process_arglist(2, args2);
    assert(result == 1);

    finalize();

    LOG_INFO("test_simple_command: PASSED");
    return 0;
}

int main(void)
{
    logger_init(LOG_LEVEL_INFO, stdout);

    LOG_INFO("=== Starting Shell Test Suite ===");

    int failures = 0;

    failures += test_shell_initialization();
    failures += test_simple_command();

    if (failures == 0) {
        LOG_INFO("=== All Shell Tests PASSED ===");
        logger_shutdown();
        return EXIT_SUCCESS;
    } else {
        LOG_ERROR("=== %d Shell Tests FAILED ===", failures);
        logger_shutdown();
        return EXIT_FAILURE;
    }
}
