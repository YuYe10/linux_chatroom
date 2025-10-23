#include "logger.h"
#include <stdarg.h>
#include <sys/stat.h>

FILE* log_file = NULL;

// 获取当前时间字符串
char* get_current_time() {
    static char time_str[64];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    return time_str;
}

// 获取日志级别字符串
const char* get_level_string(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

// 初始化日志系统
void log_init(const char* process_type, const char* username) {
    // 创建logs目录（如果不存在）
    if (mkdir("logs", 0755) == -1) {
        // 目录可能已存在，忽略错误
    }
    
    // 生成日志文件名：logs/YYYY-MM-DD_进程类型_用户名.log
    char filename[256];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(filename, sizeof(filename), "logs/%Y-%m-%d", tm_info);
    
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
    
    // 设置文件缓冲区为行缓冲
    setvbuf(log_file, NULL, _IOLBF, 0);
    
    LOG_INFO("=== 日志系统初始化成功 ===");
    LOG_INFO("进程类型: %s", process_type);
    if (username && strlen(username) > 0) {
        LOG_INFO("用户名: %s", username);
    }
    LOG_INFO("日志文件: %s", filename);
}

// 记录日志消息
void log_message(LogLevel level, const char* format, ...) {
    if (log_file == NULL) {
        return; // 日志系统未初始化
    }
    
    va_list args;
    char message[1024];
    
    // 格式化消息
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // 写入日志文件
    fprintf(log_file, "[%s] [%s] [PID:%d] %s\n", 
            get_current_time(), get_level_string(level), getpid(), message);
    
    // 对于错误和警告级别，同时输出到标准错误
    if (level >= LOG_WARNING) {
        fprintf(stderr, "[%s] [%s] %s\n", get_current_time(), get_level_string(level), message);
    }
}

// 关闭日志系统
void log_close(void) {
    if (log_file != NULL) {
        LOG_INFO("=== 日志系统关闭 ===");
        fclose(log_file);
        log_file = NULL;
    }
}