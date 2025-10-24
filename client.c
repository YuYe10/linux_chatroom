/**
 * @file client.c
 * @brief 聊天室客户端主程序
 * @details 实现客户端网络通信、消息收发、文件传输等核心功能
 * @version 1.0
 * @date 2025-10-24
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include "client_login.h"
#include "logger.h"
#include <sys/select.h>

// 全局变量定义
int sockfd;                    ///< 客户端socket文件描述符
char* IP = "127.0.0.1";       ///< 服务器IP地址（默认本地回环）
short PORT = 10222;           ///< 服务器端口号
typedef struct sockaddr SA;    ///< 套接字地址结构体别名
char username[20];            ///< 当前用户名
volatile sig_atomic_t exit_flag = 0;  ///< 退出标志（原子操作，线程安全）

// 函数声明
void service_menu();           ///< 服务菜单显示函数
void online_display(struct Msg msg);  ///< 在线用户显示函数
void filesend(int sockfd, struct Msg msg);  ///< 文件发送函数

/**
 * @brief 客户端初始化函数
 * @details 
 * - 初始化日志系统
 * - 创建TCP套接字并连接服务器
 * - 进行用户认证（注册/登录）
 * - 发送进入聊天室通知
 */
void init() {
    // 初始化日志系统（用户名稍后设置）
    log_init("client", "unknown");
    
    struct Msg login_msg;  ///< 登录消息结构体
    
    LOG_INFO("开始初始化客户端");
    
    // 创建TCP套接字
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;  ///< 服务器地址结构体
    
    // 设置服务器地址信息
    addr.sin_family = PF_INET;        ///< IPv4协议族
    addr.sin_port = htons(PORT);      ///< 端口号（网络字节序）
    addr.sin_addr.s_addr = inet_addr(IP);  ///< IP地址转换
    
    // 连接服务器
    if (connect(sockfd, (SA*)&addr, sizeof(addr)) == -1) {
        LOG_ERROR("无法连接到服务器: %s", strerror(errno));
        exit(-1);  ///< 连接失败，退出程序
    }
    
    LOG_INFO("客户端启动成功，已连接到服务器");
    
    // 获取服务器响应（登录/注册结果）
    login_msg = ask_server(sockfd);
    
    // 保存用户名
    strcpy(username, login_msg.name);
    
    // 重新初始化日志系统，使用正确的用户名
    log_close();
    log_init("client", username);
    
    // 登录成功，发送进入聊天室通知
    if (login_msg.cmd == 1001 || login_msg.cmd == 1002) {
        sprintf(login_msg.msg, "%s进入了聊天室", login_msg.name);
        LOG_INFO("用户 %s 进入聊天室", login_msg.name);
        login_msg.cmd = 0;  ///< 重置命令码为普通消息
        write(sockfd, &login_msg, sizeof(login_msg));  ///< 发送进入通知
    }
}

/**
 * @brief 文件发送函数
 * @param sockfd 客户端socket文件描述符
 * @param msg 消息结构体（用于传输文件信息）
 * @details 
 * - 获取用户输入的文件路径
 * - 提取文件名并计算文件大小
 * - 分段读取文件内容并发送
 * - 显示传输进度
 * - 发送文件结束标志
 */
void filesend(int sockfd, struct Msg msg) {
    FILE *fq;              ///< 文件指针
    char filename[100];    ///< 文件名缓冲区
    
    printf("请输入你要打开的文件名及路径，如~/1.jpg\n");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = 0;  ///< 去除换行符
    
    LOG_INFO("用户 %s 开始发送文件: %s", username, filename);
    
    // 提取文件名（不含路径）
    char *basename = strrchr(filename, '/');
    if (basename != NULL) {
        basename++;  ///< 跳过'/'
    } else {
        basename = filename;  ///< 如果没有路径，直接使用文件名
    }
    
    // 保存文件名到消息结构体
    strncpy(msg.filename, basename, sizeof(msg.filename) - 1);
    msg.filename[sizeof(msg.filename) - 1] = '\0';
    
    // 打开文件
    if ((fq = fopen(filename, "rb")) == NULL) {
        LOG_ERROR("打开文件失败: %s", filename);
        printf("打开文件%s出现错误\n", filename);
        return;  ///< 文件打开失败，直接返回
    }
    
    // 计算文件大小（优先使用fstat，失败时回退到fseek/ftell）
    struct stat st;
    long file_size = 0;
    if (fstat(fileno(fq), &st) == 0) {
        file_size = (long)st.st_size;  ///< 使用fstat获取文件大小
    } else {
        if (fseek(fq, 0, SEEK_END) == 0) {
            file_size = ftell(fq);     ///< 使用fseek/ftell获取文件大小
            fseek(fq, 0, SEEK_SET);    ///< 重置文件指针到开头
        } else {
            file_size = 0;  ///< 获取文件大小失败
        }
    }
    
    LOG_INFO("文件打开成功，开始传输: %s (大小: %ld 字节)", basename, file_size);
    
    size_t total_sent = 0;  ///< 已发送字节数
    size_t bytes_read;      ///< 每次读取的字节数
    
    // 分段读取并发送文件
    while ((bytes_read = fread(msg.file, 1, sizeof(msg.file), fq)) > 0) {
        msg.flen = bytes_read;  ///< 设置文件内容长度
        ssize_t bytes_written = write(sockfd, &msg, sizeof(msg));  ///< 发送文件数据
        
        if (bytes_written != sizeof(msg)) {
            printf("发送失败: 网络写入错误\n");
            fclose(fq);
            return;  ///< 发送失败，退出函数
        }
        
        total_sent += bytes_read;  ///< 更新已发送字节数
        
        // 显示传输进度
        if (file_size > 0) {
            int progress = (int)((total_sent * 100) / file_size);
            printf("\r传输进度: %d%% [%ld/%ld 字节]", progress, total_sent, file_size);
            fflush(stdout);  ///< 立即刷新输出缓冲区
        }
        
        // 清空文件缓冲区，避免残留数据
        memset(msg.file, 0, sizeof(msg.file));
    }
    
    // 发送文件结束标志
    msg.flen = -1;  ///< 设置文件结束标志
    ssize_t bytes_written = write(sockfd, &msg, sizeof(msg));
    if (bytes_written != sizeof(msg)) {
        LOG_ERROR("发送文件结束标志失败");
        printf("发送结束标志失败\n");
    } else {
        LOG_INFO("文件发送成功: %s，总发送字节: %zu", basename, total_sent);
        printf("\n文件发送成功: %s\n", basename);
    }
    
    fclose(fq);  ///< 关闭文件
}

/**
 * @brief 消息接收线程函数
 * @param p 线程参数（未使用）
 * @return void* 线程返回值
 * @details 
 * - 持续接收服务器发送的消息
 * - 处理普通聊天消息、文件传输消息、在线用户列表
 * - 实现文件接收功能，创建接收文件夹
 * - 处理连接断开等异常情况
 */
void* recv_thread(void* p) {
    struct Msg c_msg;      ///< 接收消息结构体
    FILE *fp = NULL;       ///< 文件指针（用于文件接收）
    int read_rec;          ///< 读取字节数
    size_t total_received = 0;  ///< 已接收字节数
    
    LOG_DEBUG("消息接收线程启动");
    
    // 初始化消息结构体
    memset(&c_msg, 0, sizeof(c_msg));
    
    while (1) {
        // 接收服务器消息
        read_rec = read(sockfd, &c_msg, sizeof(c_msg));
        
        if (read_rec <= 0) {
            // 连接断开或错误
            LOG_ERROR("与服务器的连接已断开");
            printf("与服务器的连接已断开\n");
            if (fp != NULL) {
                fclose(fp);
                fp = NULL;
            }
            break;  ///< 退出循环，结束线程
        }
        
        if (c_msg.cmd == 0) {  ///< 普通聊天消息
            LOG_DEBUG("收到聊天消息，发送者: %s，内容: %s", c_msg.name, c_msg.msg);
            printf("\n%s:%s\n", c_msg.name, c_msg.msg);
        } else if (c_msg.cmd == 4) {  ///< 文件接收
            if (c_msg.flen == -1) {  ///< 文件传输结束
                if (fp != NULL) {
                    fclose(fp);
                    fp = NULL;
                    LOG_INFO("文件接收完成，总接收字节: %zu", total_received);
                    printf("文件接收完成，共接收 %zu 字节\n", total_received);
                    total_received = 0;  ///< 重置接收字节数
                }
                continue;  ///< 继续接收下一条消息
            }
            
            // 如果是新文件传输开始
            if (fp == NULL) {
                char folder_name[50];      ///< 接收文件夹名称
                char file_path[200];       ///< 完整文件路径
                char original_filename[100];  ///< 原始文件名

                // 创建接收文件夹名称：接收者_received
                snprintf(folder_name, sizeof(folder_name), "%s_received", username);

                // 创建文件夹（如果不存在）
                if (mkdir(folder_name, 0755) == -1) {
                    // 文件夹可能已存在，忽略错误
                }
                
                // 使用发送方提供的文件名
                if (strlen(c_msg.filename) > 0) {
                    strncpy(original_filename, c_msg.filename, sizeof(original_filename) - 1);
                    original_filename[sizeof(original_filename) - 1] = '\0';
                } else {
                    // 如果发送方没有提供文件名，使用时间戳生成
                    time_t now = time(NULL);
                    struct tm *t = localtime(&now);
                    snprintf(original_filename, sizeof(original_filename),
                            "received_file_%04d%02d%02d_%02d%02d%02d",
                            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                            t->tm_hour, t->tm_min, t->tm_sec);
                }
                
                // 构建完整文件路径
                snprintf(file_path, sizeof(file_path), "%s/%s", folder_name, original_filename);
                
                // 创建接收文件
                if ((fp = fopen(file_path, "wb")) == NULL) {
                    printf("文件创建失败: %s\n", file_path);
                    continue;  ///< 文件创建失败，继续接收
                }
                
                LOG_INFO("开始接收文件，发送者: %s，文件名: %s", c_msg.name, original_filename);
                printf("\n开始接收来自 %s 的文件: %s\n", c_msg.name, original_filename);
            }
            
            // 写入文件内容
            if (fp != NULL && c_msg.flen > 0) {
                size_t bytes_written = fwrite(c_msg.file, 1, c_msg.flen, fp);
                if (bytes_written != c_msg.flen) {
                    LOG_ERROR("文件写入错误，期望写入: %d 字节，实际写入: %zu 字节", c_msg.flen, bytes_written);
                    printf("文件写入错误\n");
                    fclose(fp);
                    fp = NULL;
                    total_received = 0;
                    continue;  ///< 写入错误，继续接收
                }
                
                total_received += bytes_written;  ///< 更新已接收字节数
                LOG_DEBUG("文件接收进度: %zu 字节", total_received);
                printf("\r已接收: %zu 字节", total_received);
                fflush(stdout);  ///< 立即刷新输出
            }
        } else if (c_msg.cmd == 3) {  ///< 在线用户列表
            LOG_DEBUG("收到在线用户列表");
            // 这个命令由主循环处理，接收线程忽略
            continue;  ///< 继续接收下一条消息
        }
        
        // 重置消息结构体，避免残留数据影响
        memset(&c_msg, 0, sizeof(c_msg));
    }
    
    if (fp != NULL) {
        fclose(fp);  ///< 确保文件关闭
    }
    
    LOG_DEBUG("消息接收线程结束");
    return NULL;  ///< 线程正常结束
}

/**
 * @brief 信号处理函数
 * @param sig 信号编号
 * @details 处理Ctrl+C信号，设置退出标志
 */
void signal_handler(int sig) {
    if (sig == SIGINT) {  ///< Ctrl+C信号
        LOG_INFO("收到Ctrl+C信号，开始退出");
        exit_flag = 1;  ///< 设置退出标志
    }
}

/**
 * @brief 优雅退出函数
 * @details 
 * - 发送退出消息给服务器
 * - 关闭socket连接
 * - 记录退出日志
 */
void graceful_exit() {
    struct Msg exit_msg;  ///< 退出消息结构体
    
    LOG_INFO("用户 %s 通过Ctrl+C退出聊天室", username);
    
    // 发送退出消息给服务器
    sprintf(exit_msg.msg, "%s退出了聊天室", username);
    strcpy(exit_msg.name, username);
    exit_msg.cmd = 0;  ///< 普通消息类型
    
    // 尝试发送退出消息（如果连接仍然有效）
    if (sockfd > 0) {
        write(sockfd, &exit_msg, sizeof(exit_msg));
        close(sockfd);  ///< 关闭socket连接
        sockfd = -1;    ///< 标记socket已关闭
    }
    
    LOG_INFO("客户端退出完成");
}

/**
 * @brief 客户端主循环函数
 * @details 
 * - 设置信号处理
 * - 创建消息接收线程
 * - 处理用户输入和命令
 * - 实现服务菜单功能
 * - 处理退出逻辑
 */
void start() {
    int choose;              ///< 用户选择
    pthread_t id;           ///< 线程ID
    int create;             ///< 线程创建结果
    void* recv_thread(void*);  ///< 线程函数声明
    
    LOG_DEBUG("启动客户端主循环");
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    
    // 创建消息接收线程
    create = pthread_create(&id, 0, recv_thread, 0);
    if (0 != create) {
        LOG_ERROR("创建消息接收线程失败: %d", create);
        printf("read thread create failed.error:%d\n", create);
    } else {
        LOG_DEBUG("消息接收线程创建成功");
    }
    
    struct Msg c_msg;  ///< 消息结构体
    pthread_detach(id);  ///< 线程分离（自动回收资源）
    
    while (1) {
        // 检查退出标志
        if (exit_flag) {
            graceful_exit();
            break;  ///< 退出主循环
        }
        
        // 获取用户输入 - 使用fgets代替scanf，支持空格
        printf("请输入消息:");
        fflush(stdout);  ///< 确保提示信息立即显示
        
        // 使用select检查标准输入是否有数据，避免阻塞
        fd_set readfds;        ///< 文件描述符集合
        struct timeval timeout;  ///< 超时时间结构体
        
        FD_ZERO(&readfds);     ///< 清空文件描述符集合
        FD_SET(STDIN_FILENO, &readfds);  ///< 添加标准输入到集合
        timeout.tv_sec = 10000;  ///< 10000秒超时
        timeout.tv_usec = 0;
        
        int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
        
        if (ret == -1) {
            // select错误，检查是否是信号中断
            if (errno == EINTR) {
                if (exit_flag) {
                    graceful_exit();
                    break;
                }
                continue;  ///< 信号中断，继续循环
            }
            LOG_ERROR("select错误: %s", strerror(errno));
            continue;  ///< 继续循环
        } else if (ret == 0) {
            // 超时，检查退出标志
            if (exit_flag) {
                graceful_exit();
                break;
            }
            continue;  ///< 超时，继续循环
        }
        
        // 有输入数据
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(c_msg.msg, sizeof(c_msg.msg), stdin);
            
            // 检查fgets是否被信号中断
            if (ferror(stdin) && errno == EINTR) {
                clearerr(stdin);  ///< 清除错误状态
                if (exit_flag) {
                    graceful_exit();
                    break;
                }
                continue;  ///< 信号中断，继续循环
            }
            
            // 去除换行符
            c_msg.msg[strcspn(c_msg.msg, "\n")] = 0;
            
            // 检查是否为空消息（用户直接按回车）
            if (strlen(c_msg.msg) == 0) {
                continue;  ///< 跳过空消息，重新提示输入
            }
            
            c_msg.cmd = 0;  ///< 设置命令码为普通消息
            strcpy(c_msg.name, username);  ///< 设置发送者用户名
            
            LOG_DEBUG("用户输入消息: %s", c_msg.msg);
            
            // 处理特殊命令
            if (strcmp(c_msg.msg, "#exit") == 0) {  ///< 退出聊天室
                LOG_INFO("用户 %s 退出聊天室", username);
                sprintf(c_msg.msg, "%s退出了聊天室\n", username);
                strcpy(c_msg.name, username);
                write(sockfd, &c_msg, sizeof(c_msg));
                close(sockfd);
                break;  ///< 退出循环
            } else if (strcmp(c_msg.msg, "#clear") == 0) {  ///< 清屏
                LOG_DEBUG("用户清屏");
                system("clear");  ///< 执行清屏命令
            } else if (strcmp(c_msg.msg, "#service") == 0) {  ///< 服务菜单
                LOG_DEBUG("用户打开服务菜单");
                service_menu();  ///< 显示服务菜单
                scanf("%d", &choose);
                getchar();  ///< 清除输入缓冲区中的换行符
                
                switch (choose) {
                    case 1:  ///< 查看在线用户
                        LOG_DEBUG("用户查看在线用户列表");
                        c_msg.cmd = 3;  ///< 设置命令码为查看在线用户
                        write(sockfd, &c_msg, sizeof(c_msg));
                        read(sockfd, &c_msg, sizeof(c_msg));
                        c_msg.cmd = 0;  ///< 重置命令码
                        online_display(c_msg);  ///< 显示在线用户
                        break;
                        
                    case 2:  ///< 文件传输
                        LOG_DEBUG("用户选择文件传输");
                        c_msg.cmd = 4;  ///< 设置命令码为文件传输
                        filesend(sockfd, c_msg);
                        c_msg.cmd = 0;  ///< 重置命令码
                        break;
                        
                    case 3:  ///< 设置在线状态
                        LOG_DEBUG("用户设置在线状态");
                        c_msg.cmd = 5;  ///< 设置命令码为在线状态
                        strcpy(c_msg.name, username);
                        write(sockfd, &c_msg, sizeof(c_msg));
                        c_msg.cmd = 0;  ///< 重置命令码
                        printf("ok.\n");
                        break;
                        
                    case 4:  ///< 设置隐身状态
                        LOG_DEBUG("用户设置隐身状态");
                        c_msg.cmd = 6;  ///< 设置命令码为隐身状态
                        strcpy(c_msg.name, username);
                        write(sockfd, &c_msg, sizeof(c_msg));
                        c_msg.cmd = 0;  ///< 重置命令码
                        printf("ok.\n");
                        break;
                }
            } else {
                // 发送普通聊天消息
                LOG_DEBUG("发送聊天消息: %s", c_msg.msg);
                write(sockfd, &c_msg, sizeof(c_msg));
            }
        }
    }
    
    LOG_INFO("客户端主循环结束");
}

/**
 * @brief 主函数
 * @return int 程序退出码
 * @details 程序入口点，初始化客户端并启动主循环
 */
int main() {
    // 设置信号处理，确保在程序开始时就能捕获Ctrl+C
    signal(SIGINT, signal_handler);
    
    init();   ///< 初始化客户端
    start();  ///< 启动主循环
    
    // 程序结束时关闭日志系统
    log_close();
    return 0;  ///< 正常退出
}

/**
 * @brief 服务菜单显示函数
 * @details 打印客户端服务选项菜单
 */
void service_menu() {
    printf("===       选项      ===\n");
    printf("    1.查看用户状态\n");
    printf("    2.传输文件\n");
    printf("    3.设置在线\n");
    printf("    4.设置隐身\n");
    printf("===                ===\n");
}

/**
 * @brief 在线用户显示函数
 * @param disp_msg 包含在线用户列表的消息结构体
 * @details 格式化显示当前在线用户列表
 */
void online_display(struct Msg disp_msg) {
    int m = 0;  ///< 循环计数器
    printf("============当前聊天室在线用户============\n");
    
    // 格式化显示在线用户列表（每行3个用户）
    for (m = 0; m < 3; m++) {
        printf(" %-10s ", disp_msg.online[m]);  ///< 左对齐显示用户名
    }
    
    for (m = 3; m < disp_msg.len + 3; m++) {
        if (m % 3 == 0)
            printf("\n");  ///< 每3个用户换行
        printf("  %-10s ", disp_msg.online[m]);  ///< 左对齐显示用户名
    }
    
    printf("\n");
    printf("========================================\n");
}