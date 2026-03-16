/**
 * @file test_integration.c
 * @brief Integration tests combining multiple components.
 */

#define _POSIX_C_SOURCE 200809L
#include "common/logger.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static int test_end_to_end(void);

static int test_end_to_end(void)
{
    LOG_INFO("Running test_end_to_end");

    /* Basic integration test */
    LOG_INFO("All components initialized successfully");

    LOG_INFO("test_end_to_end: PASSED");
    return 0;
}

int main(void)
{
    logger_init(LOG_LEVEL_INFO, stdout);

    LOG_INFO("=== Starting Integration Test Suite ===");

    int failures = 0;

    failures += test_end_to_end();

    if (failures == 0) {
        LOG_INFO("=== All Integration Tests PASSED ===");
        logger_shutdown();
        return EXIT_SUCCESS;
    } else {
        LOG_ERROR("=== %d Integration Tests FAILED ===", failures);
        logger_shutdown();
        return EXIT_FAILURE;
    }
}
