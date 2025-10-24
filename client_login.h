/**
 * @file client_login.h
 * @brief 客户端登录认证模块头文件
 * @details 定义客户端与服务器通信的消息结构体和函数声明
 * @version 1.0
 * @date 2025-10-24
 */

#include <stdio.h>
#define MAXLINE 1024  ///< 最大消息长度定义

/**
 * @brief 客户端与服务器通信的消息结构体
 * @details 用于封装客户端与服务器之间的所有通信数据
 */
struct Msg {
    char online[303][20];  ///< 在线用户列表数组，最多支持303个用户
    int len;               ///< 在线用户数量
    char msg[1024];        ///< 消息内容缓冲区
    char file[MAXLINE];    ///< 文件内容缓冲区
    int flen;              ///< 文件内容长度
    char name[20];         ///< 用户账号
    char password[20];     ///< 用户密码
    char filename[100];    ///< 文件名
    int cmd;               ///< 消息类型/命令码
};

/**
 * @brief 显示登录/注册界面
 * @details 打印用户选择菜单
 */
void Interface();

/**
 * @brief 用户注册函数
 * @param sockfd 客户端socket文件描述符
 * @details 处理用户注册逻辑，向服务器发送注册请求
 */
void Reg(int sockfd);

/**
 * @brief 用户登录函数
 * @param sockfd 客户端socket文件描述符
 * @details 处理用户登录逻辑，向服务器发送登录请求
 */
void Entry(int sockfd);

/**
 * @brief 客户端与服务器认证交互函数
 * @param sockfd 客户端socket文件描述符
 * @return struct Msg 服务器返回的认证结果消息
 * @details 处理用户注册/登录选择，与服务器进行认证交互
 */
struct Msg ask_server(int sockfd);