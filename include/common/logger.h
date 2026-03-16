/**
 * @file logger.h
 * @brief Production-grade logging system for system programming applications.
 *
 * Provides structured logging with configurable log levels, thread-safe operations,
 * and context propagation. Designed for high-performance system-level applications.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

/**
 * @brief Log severity levels following industry standard practices.
 */
typedef enum {
    LOG_LEVEL_TRACE = 0,   /**< Detailed diagnostic information */
    LOG_LEVEL_DEBUG = 1,   /**< Debug information for development */
    LOG_LEVEL_INFO = 2,    /**< Informational messages */
    LOG_LEVEL_WARN = 3,    /**< Warning messages for recoverable issues */
    LOG_LEVEL_ERROR = 4,   /**< Error messages for failures */
    LOG_LEVEL_FATAL = 5    /**< Fatal errors requiring termination */
} log_level_t;

/**
 * @brief Initialize the logging system.
 *
 * @param level Minimum log level to output
 * @param output File handle for log output (use stderr or stdout)
 * @return 0 on success, -1 on failure
 */
int logger_init(log_level_t level, FILE *output);

/**
 * @brief Shutdown the logging system.
 */
void logger_shutdown(void);

/**
 * @brief Set the current log level.
 *
 * @param level New minimum log level
 */
void logger_set_level(log_level_t level);

/**
 * @brief Log a message with file, line, and function context.
 *
 * @param level Severity level of the message
 * @param file Source file name
 * @param line Line number
 * @param func Function name
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void logger_log(log_level_t level, const char *file, int line,
                const char *func, const char *format, ...);

/**
 * @brief Log a system error with errno context.
 *
 * @param level Severity level
 * @param file Source file name
 * @param line Line number
 * @param func Function name
 * @param syscall Name of the system call that failed
 */
void logger_log_errno(log_level_t level, const char *file, int line,
                      const char *func, const char *syscall);

/* Convenience macros for logging */
#define LOG_TRACE(...) logger_log(LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(...) logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(...)  logger_log(LOG_LEVEL_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(...)  logger_log(LOG_LEVEL_WARN,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...) logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_FATAL(...) logger_log(LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_ERRNO(level, syscall) logger_log_errno(level, __FILE__, __LINE__, __func__, syscall)

#endif /* LOGGER_H */
