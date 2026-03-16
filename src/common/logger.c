/**
 * @file logger.c
 * @brief Implementation of production-grade logging system.
 */

#define _POSIX_C_SOURCE 200809L
#include "common/logger.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static log_level_t current_level = LOG_LEVEL_INFO;
static FILE *log_output = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static int initialized = 0;

static const char *level_strings[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char *level_colors[] = {
    "\x1b[94m",  /* TRACE: Light Blue */
    "\x1b[36m",  /* DEBUG: Cyan */
    "\x1b[32m",  /* INFO: Green */
    "\x1b[33m",  /* WARN: Yellow */
    "\x1b[31m",  /* ERROR: Red */
    "\x1b[35m"   /* FATAL: Magenta */
};

static const char *color_reset = "\x1b[0m";

int logger_init(log_level_t level, FILE *output)
{
    pthread_mutex_lock(&log_mutex);

    if (initialized) {
        pthread_mutex_unlock(&log_mutex);
        return 0;  /* Already initialized */
    }

    current_level = level;
    log_output = output ? output : stderr;
    initialized = 1;

    pthread_mutex_unlock(&log_mutex);
    return 0;
}

void logger_shutdown(void)
{
    pthread_mutex_lock(&log_mutex);
    initialized = 0;
    pthread_mutex_unlock(&log_mutex);
}

void logger_set_level(log_level_t level)
{
    pthread_mutex_lock(&log_mutex);
    current_level = level;
    pthread_mutex_unlock(&log_mutex);
}

void logger_log(log_level_t level, const char *file, int line,
                const char *func, const char *format, ...)
{
    if (!initialized) {
        logger_init(LOG_LEVEL_INFO, stderr);
    }

    if (level < current_level) {
        return;
    }

    pthread_mutex_lock(&log_mutex);

    /* Get current timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm *tm_info = localtime(&ts.tv_sec);

    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    /* Check if output is a TTY for color support */
    int use_color = isatty(fileno(log_output));

    /* Print log header */
    if (use_color) {
        fprintf(log_output, "%s[%s.%03ld] [%s%-5s%s] [%s:%d] %s: ",
                color_reset, time_buf, ts.tv_nsec / 1000000,
                level_colors[level], level_strings[level], color_reset,
                file, line, func);
    } else {
        fprintf(log_output, "[%s.%03ld] [%-5s] [%s:%d] %s: ",
                time_buf, ts.tv_nsec / 1000000,
                level_strings[level],
                file, line, func);
    }

    /* Print log message */
    va_list args;
    va_start(args, format);
    vfprintf(log_output, format, args);
    va_end(args);

    fprintf(log_output, "\n");
    fflush(log_output);

    pthread_mutex_unlock(&log_mutex);
}

void logger_log_errno(log_level_t level, const char *file, int line,
                      const char *func, const char *syscall)
{
    int saved_errno = errno;
    char err_buf[256];
    strerror_r(saved_errno, err_buf, sizeof(err_buf));

    logger_log(level, file, line, func, "%s failed: %s (errno=%d)",
               syscall, err_buf, saved_errno);

    errno = saved_errno;  /* Restore errno */
}
