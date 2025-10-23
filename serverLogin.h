#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#define MAXLINE 1024  // 最大消息长度

// 消息结构体定义
struct Msg {
    char online[303][20];  // 在线用户列表，最多303个用户，每个用户名最多20字符
    int len;               // 在线用户数量
    char msg[1024];        // 消息内容
    char file[MAXLINE];    // 文件内容
    int flen;              // 文件长度
    char name[20];         // 用户账号
    char password[20];     // 用户密码
    char filename[100];    // 新增：文件名
    int cmd;               // 消息类型/命令
};

// 用户注册功能
void Reg(int client_socket, struct Msg msg, MYSQL *con);

// 用户登录功能
void Entry(int client_socket, struct Msg msg, MYSQL *con);