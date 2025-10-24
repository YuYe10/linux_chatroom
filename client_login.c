/**
 * @file client_login.c
 * @brief 客户端登录认证模块实现文件
 * @details 实现用户注册、登录及与服务器认证交互功能
 * @version 1.0
 * @date 2025-10-24
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "client_login.h"

struct Msg msg;  ///< 全局消息结构体变量

/**
 * @brief 显示用户认证界面
 * @details 打印注册/登录选择菜单
 */
void Interface() {
    printf("===     选项    ===\n");
    printf ("     1. 注册\n");
    printf ("     2. 登录\n");
    printf("===            ===\n");
}

/**
 * @brief 用户注册函数
 * @param sockfd 客户端socket文件描述符
 * @details 
 * - 设置消息内容为"#service"标识服务请求
 * - 获取用户输入的用户名和密码
 * - 设置命令码为1（注册）
 * - 向服务器发送注册请求并等待响应
 * - 处理注册结果：成功(1001)或失败(-1)
 */
void Reg(int sockfd)
{
    char name[20];        ///< 用户名输入缓冲区
    char password[20];    ///< 密码输入缓冲区
    
    strcpy(msg.msg,"#service");  ///< 设置消息内容为服务标识

    printf("用户名:");
    scanf("%s",name);
    while(getchar()!='\n');  ///< 清除输入缓冲区
    strcpy(msg.name,name);   ///< 复制用户名到消息结构体

    printf("密码:");
    scanf("%s",password);
    while(getchar()!='\n');  ///< 清除输入缓冲区
    strcpy(msg.password,password);  ///< 复制密码到消息结构体
    
    msg.cmd=1;  ///< 设置命令码为1（注册）
    
    // 向服务器发送注册请求
    write(sockfd,&msg,sizeof(msg));
    // 等待服务器响应
    read(sockfd,&msg,sizeof(msg));

    printf("msg.cmd = %d\n", msg.cmd);  ///< 打印服务器响应码

    if(msg.cmd==1001){  ///< 注册成功
        printf("Login success!waiting...\n");
    }
    else if(msg.cmd==-1){  ///< 注册失败（用户名已被使用）
        printf("the UserName has been used.\n");
        printf("returning...\n");
        sleep(1);  ///< 等待1秒后返回
    }
}

/**
 * @brief 用户登录函数
 * @param sockfd 客户端socket文件描述符
 * @details 
 * - 设置消息内容为"#service"标识服务请求
 * - 获取用户输入的用户名和密码
 * - 设置命令码为2（登录）
 * - 向服务器发送登录请求并等待响应
 * - 处理登录结果：成功(1002)或失败(-1)
 */
void Entry(int sockfd)
{
    char name[20];        ///< 用户名输入缓冲区
    char password[20];    ///< 密码输入缓冲区
    
    strcpy(msg.msg,"#service");  ///< 设置消息内容为服务标识

    printf("用户名:");
    scanf("%s",name);
    while(getchar()!='\n');  ///< 清除输入缓冲区
    strcpy(msg.name,name);   ///< 复制用户名到消息结构体
    
    printf("密码:");
    scanf("%s",password);
    while(getchar()!='\n');  ///< 清除输入缓冲区
    strcpy(msg.password,password);  ///< 复制密码到消息结构体
    
    msg.cmd=2;  ///< 设置命令码为2（登录）

    // 向服务器发送登录请求
    write(sockfd,&msg,sizeof(msg));
    // 等待服务器响应
    read(sockfd,&msg,sizeof(msg));

    printf("msg.cmd = %d\n", msg.cmd);  ///< 打印服务器响应码
    
    if(msg.cmd==-1){  ///< 登录失败
        printf("Fail To Login ,Please Resume Load...\n");
    }
    else if(msg.cmd==1002){  ///< 登录成功
        printf("Login success ! waiting...\n");
    }
}

/**
 * @brief 客户端认证交互主函数
 * @param sockfd 客户端socket文件描述符
 * @return struct Msg 服务器返回的认证结果消息
 * @details 
 * - 显示认证界面供用户选择注册或登录
 * - 根据用户选择调用相应函数
 * - 处理认证结果并返回服务器响应
 */
struct Msg ask_server(int sockfd)
{
    char ch[2];  ///< 用户选择输入缓冲区
    
    while (1) {
        Interface();  ///< 显示认证界面
        scanf("%c",ch);
        while(getchar()!= '\n');  ///< 清除输入缓冲区
        
        switch(ch[0]) {
            case '1':  ///< 用户选择注册
                Reg(sockfd);
                break;
            case '2':  ///< 用户选择登录
                Entry(sockfd);
                break;
        }
        
        sleep(2);      ///< 等待2秒
        system("clear");  ///< 清屏
        return msg;    ///< 返回服务器响应消息
    }
}