/**
 * @file logger.c
 * @brief 日志系统模块实现文件
 * @details 实现日志系统的初始化、日志记录和关闭功能
 */

#include "logger.h"
#include <stdarg.h>
#include <sys/stat.h>

// 全局日志文件指针
FILE* log_file = NULL;

/**
 * @brief 获取当前时间字符串
 * @return 格式化的时间字符串（YYYY-MM-DD HH:MM:SS）
 */
char* get_current_time() {
    static char time_str[64];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    // 格式化时间：年-月-日 时:分:秒
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    return time_str;
}

/**
 * @brief 获取日志级别对应的字符串
 * @param level 日志级别
 * @return 日志级别字符串
 */
const char* get_level_string(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

/**
 * @brief 初始化日志系统
 * @param process_type 进程类型（如"server"、"client"）
 * @param username 用户名（可选，可为NULL）
 * @note 创建logs目录，生成带时间戳的日志文件名
 */
void log_init(const char* process_type, const char* username) {
    // 创建logs目录（如果不存在），权限为755
    if (mkdir("logs", 0755) == -1) {
        // 目录可能已存在，忽略错误
    }
    
    // 生成日志文件名：logs/YYYY-MM-DD_进程类型_用户名.log
    char filename[256];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    // 格式化日期部分
    strftime(filename, sizeof(filename), "logs/%Y-%m-%d", tm_info);
    
    // 根据是否有用户名添加不同的后缀
    if (username && strlen(username) > 0) {
        snprintf(filename + strlen(filename), sizeof(filename) - strlen(filename), 
                "_%s_%s.log", process_type, username);
    } else {
        snprintf(filename + strlen(filename), sizeof(filename) - strlen(filename), 
                "_%s.log", process_type);
    }
    
    // 打开日志文件（追加模式）
    log_file = fopen(filename, "a");
    if (log_file == NULL) {
        fprintf(stderr, "无法打开日志文件: %s\n", filename);
        return;
    }
    
    // 设置文件缓冲区为行缓冲（每行立即写入）
    setvbuf(log_file, NULL, _IOLBF, 0);
    
    // 记录初始化成功日志
    LOG_INFO("=== 日志系统初始化成功 ===");
    LOG_INFO("进程类型: %s", process_type);
    if (username && strlen(username) > 0) {
        LOG_INFO("用户名: %s", username);
    }
    LOG_INFO("日志文件: %s", filename);
}

/**
 * @brief 记录日志消息
 * @param level 日志级别
 * @param format 格式化字符串
 * @param ... 可变参数
 * @note 日志格式：[时间] [级别] [PID:进程ID] 消息内容
 */
void log_message(LogLevel level, const char* format, ...) {
    // 检查日志系统是否已初始化
    if (log_file == NULL) {
        return; // 日志系统未初始化
    }
    
    va_list args;
    char message[1024];
    
    // 使用可变参数格式化消息内容
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // 写入日志文件：时间 + 级别 + 进程ID + 消息
    fprintf(log_file, "[%s] [%s] [PID:%d] %s\n", 
            get_current_time(), get_level_string(level), getpid(), message);
    
    // 对于错误和警告级别，同时输出到标准错误流
    if (level >= LOG_WARNING) {
        fprintf(stderr, "[%s] [%s] %s\n", get_current_time(), get_level_string(level), message);
    }
}

/**
 * @brief 关闭日志系统
 * @note 关闭日志文件并释放资源
 */
void log_close(void) {
    if (log_file != NULL) {
        LOG_INFO("=== 日志系统关闭 ===");
        fclose(log_file);
        log_file = NULL;
    }
}