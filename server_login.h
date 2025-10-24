/**
 * @file server_login.h
 * @brief 服务器登录认证模块头文件
 * @details 定义消息结构体和登录认证相关函数声明
 */

#ifndef SERVER_LOGIN_H
#define SERVER_LOGIN_H

#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>

#define MAXLINE 1024  // 最大消息长度

/**
 * @brief 消息结构体定义
 * @details 用于客户端和服务器之间的消息传递
 */
struct Msg {
    char online[303][20];  // 在线用户列表，最多303个用户，每个用户名最多20字符
    int len;               // 在线用户数量
    char msg[1024];        // 消息内容
    char file[MAXLINE];    // 文件内容
    int flen;              // 文件长度
    char name[20];         // 用户账号
    char password[20];     // 用户密码
    char filename[100];    // 文件名
    int cmd;               // 消息类型/命令
};

/**
 * @brief 用户注册功能
 * @param client_socket 客户端socket文件描述符
 * @param msg 消息结构体
 * @param con MySQL数据库连接句柄
 */
void Reg(int client_socket, struct Msg msg, MYSQL *con);

/**
 * @brief 用户登录功能
 * @param client_socket 客户端socket文件描述符
 * @param msg 消息结构体
 * @param con MySQL数据库连接句柄
 */
void Entry(int client_socket, struct Msg msg, MYSQL *con);

#endif