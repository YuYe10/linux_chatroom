/**
 * @file logger.h
 * @brief 日志系统模块头文件
 * @details 定义日志级别、日志函数声明和便捷宏
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

/**
 * @brief 日志级别枚举
 * @details 定义四种日志级别，数值越小级别越低
 */
typedef enum {
    LOG_DEBUG = 0,   // 调试级别，用于开发调试
    LOG_INFO = 1,    // 信息级别，用于一般信息记录
    LOG_WARNING = 2, // 警告级别，用于警告信息
    LOG_ERROR = 3    // 错误级别，用于错误信息
} LogLevel;

// 日志文件指针（全局变量）
extern FILE* log_file;

/**
 * @brief 初始化日志系统
 * @param process_type 进程类型（如"server"、"client"）
 * @param username 用户名（可选，可为NULL）
 */
void log_init(const char* process_type, const char* username);

/**
 * @brief 记录日志消息
 * @param level 日志级别
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void log_message(LogLevel level, const char* format, ...);

/**
 * @brief 关闭日志系统
 */
void log_close(void);

// 便捷宏定义，用于直接调用不同级别的日志函数
#define LOG_DEBUG(...) log_message(LOG_DEBUG, __VA_ARGS__)    // 调试日志宏
#define LOG_INFO(...) log_message(LOG_INFO, __VA_ARGS__)      // 信息日志宏
#define LOG_WARNING(...) log_message(LOG_WARNING, __VA_ARGS__) // 警告日志宏
#define LOG_ERROR(...) log_message(LOG_ERROR, __VA_ARGS__)    // 错误日志宏

#endif