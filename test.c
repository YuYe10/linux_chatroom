/**
 * @file test.c
 * @brief 测试文件
 * @details 用于测试数据库连接和文件操作功能
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "database.h"
#include "server_login.h"

/**
 * @brief 主函数
 * @return 程序退出状态
 * @details 测试环境变量读取和文件操作
 */
int main() {
    /* 
    // 测试代码：从环境变量获取数据库密码
    char* db_pwd = getenv("CHATROOM_DB_PASS");
    if (db_pwd == NULL) {
        fprintf(stderr, "Database password environment variable not set\n");
        return 1;
    }
    printf("Database password: %s\n", db_pwd);
    */

    // 测试文件操作：打开或创建测试文件
    FILE *fp;
    fp = fopen("～/testfile.txt", "a+");  // 以追加读写模式打开文件
    
    // 注意：文件路径中的波浪号可能需要展开为完整路径
    // 建议使用绝对路径或相对路径，如"./testfile.txt"
    
    return 0;
}