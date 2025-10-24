#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
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
#include "logger.h"  // 包含日志头文件

/**服务器端主程序
   采用多线程机制为多个客户端提供服务*/

// 全局变量定义
int sockfd;                    // 服务器socket文件描述符
int fds[100];                  // 客户端socket文件描述符数组，最多支持100个客户端
int size = 100;                // 最大客户端连接数
int online_count = 0;          // 当前在线客户端数量
char* IP = "127.0.0.1";       // 服务器IP地址
short PORT = 10222;            // 服务器端口号
typedef struct sockaddr SA;    // 套接字地址结构体别名

MYSQL *mysql_handle = NULL;    // 数据库连接句柄
struct Msg msg;                // 消息结构体
struct sockaddr_in addr;       // 套接字地址结构

// 函数声明
void SendMsgToAll(struct Msg msg, int fd);
void connect_db();

/**服务器初始化函数
   创建socket，绑定地址，监听端口，连接数据库*/
void init() {
    // 初始化日志系统
    log_init("server", NULL);
    LOG_INFO("开始初始化服务器");
    
    // 创建TCP套接字
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        LOG_ERROR("创建socket失败: %s", strerror(errno));
        exit(-1);
    }
    LOG_DEBUG("TCP套接字创建成功");
    
    // 设置服务器地址信息
    addr.sin_family = PF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP);
    
    // 绑定套接字到指定地址
    if (bind(sockfd, (SA*)&addr, sizeof(addr)) == -1) {
        perror("绑定失败");
        exit(-1);
    }
    
    // 开始监听连接请求，最大连接数为100
    if (listen(sockfd, 100) == -1) {
        perror("设置监听失败");
        exit(-1);
    }
    
    // 连接数据库
    connect_db();
    LOG_INFO("服务器初始化完成");
}

/**数据库连接函数
   初始化并连接MySQL数据库，创建必要的表*/
void connect_db() {
    LOG_INFO("开始连接数据库");
    
    // 初始化数据库连接
    mysql_handle = db_initial(mysql_handle);
    if (NULL == mysql_handle) {
        LOG_ERROR("数据库初始化失败");
        exit(1);
    }
    
    // 从环境变量获取密码
    char* db_pwd = getenv("CHATROOM_DB_PASS");
    if (db_pwd == NULL) {
        fprintf(stderr, "Database password environment variable not set\n");
        exit(1);
    }
    // 先连接到MySQL服务器（不指定数据库）
    MYSQL *temp_conn = mysql_real_connect(mysql_handle, "localhost", "root", db_pwd, NULL, 3306, NULL, 0);
    if (temp_conn == NULL) {
        fprintf(stderr, "Connection to MySQL server failed: %s\n", mysql_error(mysql_handle));
        exit(1);
    }
    printf("Connected to MySQL server successfully!\n");

    // 检查chatroom数据库是否存在
    MYSQL_RES *result = mysql_list_dbs(mysql_handle, "chatroom");
    if (result == NULL) {
        fprintf(stderr, "Error checking databases: %s\n", mysql_error(mysql_handle));
        exit(1);
    }

    int db_exists = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        if (row[0] && strcmp(row[0], "chatroom") == 0) {
            db_exists = 1;
            break;
        }
    }
    mysql_free_result(result);

    // 如果数据库不存在，则创建它
    if (!db_exists) {
        printf("Database 'chatroom' does not exist. Creating it...\n");
        db_create(mysql_handle, "create database chatroom");
        printf("Database 'chatroom' created successfully!\n");
    }

    // 关闭临时连接并重新连接到chatroom数据库
    mysql_close(mysql_handle);
    mysql_handle = db_initial(mysql_handle);
    if (NULL == mysql_handle) {
        fprintf(stderr, "%s\n", "Re-initialize failed.");
        exit(1);
    }

    // 连接到MySQL数据库
    mysql_handle = db_connect(mysql_handle, "localhost", "root", db_pwd, "chatroom");
    
    // 创建用户信息表
    table_create(mysql_handle, "chatroom",
                "create table usrinfo(userID INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT, userName TEXT, userPWD TEXT) engine=INNODB auto_increment=1001 default charset=gbk");
    
    // 创建在线用户信息表
    table_create(mysql_handle, "chatroom",
                "create table onlineinfo(userID INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT, userName TEXT, userState TEXT) engine=INNODB auto_increment=1001 default charset=gbk");
    
    // 删除usrinfo表中username为空的记录
    char delete_query[1024];
    sprintf(delete_query, "DELETE FROM usrinfo WHERE userName = '' OR userName IS NULL");
    if (mysql_real_query(mysql_handle, delete_query, (unsigned int)strlen(delete_query)) != 0) {
        LOG_ERROR("删除usrinfo表空用户名记录失败: %s", mysql_error(mysql_handle));
    } else {
        int affected_rows = mysql_affected_rows(mysql_handle);
        if (affected_rows > 0) {
            LOG_INFO("成功删除usrinfo表 %d 条空用户名记录", affected_rows);
        } else {
            LOG_DEBUG("usrinfo表未找到空用户名记录");
        }
    }
    
    // 删除onlineinfo表中username为空的记录
    sprintf(delete_query, "DELETE FROM onlineinfo WHERE userName = '' OR userName IS NULL");
    if (mysql_real_query(mysql_handle, delete_query, (unsigned int)strlen(delete_query)) != 0) {
        LOG_ERROR("删除onlineinfo表空用户名记录失败: %s", mysql_error(mysql_handle));
    } else {
        int affected_rows = mysql_affected_rows(mysql_handle);
        if (affected_rows > 0) {
            LOG_INFO("成功删除onlineinfo表 %d 条空用户名记录", affected_rows);
        } else {
            LOG_DEBUG("onlineinfo表未找到空用户名记录");
        }
    }
    
    LOG_INFO("数据库连接完成");
}

/**服务线程函数
   为每个客户端连接创建独立的服务线程*/
void* service_thread(void* p) {
    int fd = *(int*)p;  // 客户端socket文件描述符
    
    LOG_INFO("创建服务线程，客户端fd=%d", fd);
    
    while (1) {
        // 接收客户端消息
        int ret = read(fd, &msg, sizeof(msg));
        
        if (ret == -1) {
            LOG_ERROR("读取客户端消息失败，fd=%d: %s", fd, strerror(errno));
            break;
        } else if (ret == 0) {  // 客户端断开连接
            LOG_INFO("客户端断开连接，fd=%d，用户名: %s", fd, msg.name);
            // 更新在线状态为离线
            data_delete_online(mysql_handle, msg.name);
            data_insert_online(mysql_handle, msg.name, "offline");
            pthread_exit(0);
            online_count--;  // 在线客户端数量减1
            break;
        } else if (strcmp(msg.msg, "#service") == 0 && (msg.cmd > 0 && msg.cmd < 7)) {
            // 处理服务命令
            switch (msg.cmd) {
                case 1:  // 用户注册
                    LOG_INFO("处理用户注册请求，用户名: %s", msg.name);
                    Reg(fd, msg, mysql_handle);
                    break;
                    
                case 2:  // 用户登录
                    LOG_INFO("处理用户登录请求，用户名: %s", msg.name);
                    Entry(fd, msg, mysql_handle);
                    break;
                    
                case 3:  // 查看在线用户
                    LOG_DEBUG("处理查看在线用户请求，用户名: %s", msg.name);
                    int len = table_display(mysql_handle, "select * from onlineinfo", msg.online);
                    msg.len = len;
                    write(fd, &msg, sizeof(msg));
                    break;
                    
                case 4:  // 文件传输
                    LOG_INFO("开始文件传输，文件名: %s，发送者: %s", msg.filename, msg.name);
                    while (1) {
                        if (msg.flen == -1) {
                            SendMsgToAll(msg, fd);
                            LOG_INFO("文件传输完成，文件名: %s", msg.filename);
                            break;
                        } else {
                            SendMsgToAll(msg, fd);
                            read(fd, &msg, sizeof(msg));
                        }
                    }
                    break;
                    
                case 5:  // 设置在线状态
                    LOG_DEBUG("设置用户在线状态，用户名: %s", msg.name);
                    data_delete_online(mysql_handle, msg.name);
                    data_insert_online(mysql_handle, msg.name, "online");
                    break;
                    
                case 6:  // 设置隐身状态
                    LOG_DEBUG("设置用户隐身状态，用户名: %s", msg.name);
                    data_delete_online(mysql_handle, msg.name);
                    data_insert_online(mysql_handle, msg.name, "offline");
                    break;
            }
        } else if (strcmp(msg.msg, "#service") && strcmp(msg.msg, "") && strcmp(msg.msg, "#clear") && msg.cmd == 0) {
            // 普通聊天消息，广播给所有客户端
            LOG_DEBUG("广播聊天消息，发送者: %s，内容: %s", msg.name, msg.msg);
            SendMsgToAll(msg, fd);
        }
    }
    
    return NULL;
}

/**主服务函数
   接受客户端连接并创建服务线程*/
void service() {
    LOG_INFO("服务器启动，开始接受客户端连接");
    
    while (1) {
        struct sockaddr_in fromaddr;
        socklen_t len = sizeof(fromaddr);
        
        // 接受客户端连接
        int fd = accept(sockfd, (SA*)&fromaddr, &len);
        if (fd == -1) {
            LOG_ERROR("接受客户端连接失败: %s", strerror(errno));
            continue;
        }
        
        LOG_INFO("新客户端连接，fd=%d", fd);
        
        // 寻找空闲位置存储客户端socket
        int i = 0;
        for (i = 0; i < size; i++) {
            if (fds[i] == 0) {
                fds[i] = fd;  // 记录客户端socket
                LOG_DEBUG("客户端fd=%d存储在位置%d", fd, i);
                
                // 创建服务线程
                pthread_t tid;
                pthread_create(&tid, 0, service_thread, &fd);
                pthread_detach(tid);  // 线程分离，自动回收资源
                online_count++;  // 在线客户端数量加1
                LOG_INFO("创建服务线程成功，当前在线客户端数: %d", online_count);
                break;
            }
            
            if (size == i) {
                // 聊天室已满，拒绝连接
                LOG_WARNING("聊天室已满，拒绝客户端连接，fd=%d", fd);
                char* str = "对不起，聊天室已经满了!";
                send(fd, str, strlen(str), 0);
                close(fd);
            }
        }
    }
}

/**主函数
   程序入口点*/
int main() {
    init();     // 初始化服务器
    service();  // 启动服务
    
    // 程序结束时关闭日志系统
    log_close();
    return 0;
}

/**向所有客户端发送消息
   将消息广播给所有连接的客户端（除了发送者自己）*/
void SendMsgToAll(struct Msg msg, int fd) {
    LOG_DEBUG("开始广播消息，发送者fd=%d，消息内容: %s", fd, msg.msg);
    
    int i;
    for (i = 0; i < size; i++) {
        if (fds[i] != 0 && fds[i] != fd) {  // 不发送给发送者自己
            LOG_DEBUG("向客户端fd=%d发送消息", fds[i]);
            write(fds[i], &msg, sizeof(msg));
        }
    }
    
    LOG_DEBUG("消息广播完成");
}
