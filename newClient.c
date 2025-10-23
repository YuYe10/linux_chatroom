#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "clientLogin.h"

// 全局变量定义
int sockfd;                    // 客户端socket文件描述符
char* IP = "127.0.0.1";       // 服务器IP地址
short PORT = 10222;            // 服务器端口号
typedef struct sockaddr SA;    // 套接字地址结构体别名
char username[20];            // 当前用户名

// 函数声明
void service_menu();
void online_display(struct Msg msg);
void filesend(int sockfd, struct Msg msg);

/**客户端初始化函数
   连接服务器并进行登录/注册*/
void init() {
    struct Msg login_msg;
    
    // 创建TCP套接字
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    
    // 设置服务器地址信息
    addr.sin_family = PF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP);
    
    // 连接服务器
    if (connect(sockfd, (SA*)&addr, sizeof(addr)) == -1) {
        perror("无法连接到服务器");
        exit(-1);
    }
    
    printf("客户端启动成功\n");
    
    // 获取服务器响应（登录/注册结果）
    login_msg = ask_server(sockfd);
    
    // 保存用户名
    strcpy(username, login_msg.name);
    
    // 登录成功，发送进入聊天室通知
    if (login_msg.cmd == 1001 || login_msg.cmd == 1002) {
        sprintf(login_msg.msg, "%s进入了聊天室", login_msg.name);
        printf("%s进入了聊天室\n", login_msg.name);
        login_msg.cmd = 0;
        write(sockfd, &login_msg, sizeof(login_msg));
    }
}

/**客户端主循环函数
   处理用户输入和消息接收*/
void start() {
    int choose;
    pthread_t id;
    int create;
    void* recv_thread(void*);
    
    // 创建消息接收线程
    create = pthread_create(&id, 0, recv_thread, 0);
    if (0 != create) {
        printf("read thread create failed.error:%d\n", create);
    }
    
    struct Msg c_msg;  // 消息结构体
    pthread_detach(id);  // 线程分离
    
    // 增加等待时间，确保登录成功消息处理完成
    usleep(200000);  // 等待200ms，确保接收线程初始化完成
    
    while (1) {
        // 获取用户输入 - 使用fgets代替scanf，支持空格
        printf("请输入消息:");
        fflush(stdout);  // 确保提示信息立即显示
        
        fgets(c_msg.msg, sizeof(c_msg.msg), stdin);
        fflush(stdin);
        
        // 去除换行符
        c_msg.msg[strcspn(c_msg.msg, "\n")] = 0;
        
        // 检查是否为空消息（用户直接按回车）
        if (strlen(c_msg.msg) == 0) {
            continue;  // 跳过空消息，重新提示输入
        }
        
        c_msg.cmd = 0;
        strcpy(c_msg.name, username);
        
        // 处理特殊命令
        if (strcmp(c_msg.msg, "#exit") == 0) {  // 退出聊天室
            sprintf(c_msg.msg, "%s退出了聊天室\n", username);
            strcpy(c_msg.name, username);
            write(sockfd, &c_msg, sizeof(c_msg));
            close(sockfd);
            break;
        } else if (strcmp(c_msg.msg, "#clear") == 0) {  // 清屏
            system("clear");
        } else if (strcmp(c_msg.msg, "#service") == 0) {  // 服务菜单
            service_menu();
            scanf("%d", &choose);
            getchar();  // 清除输入缓冲区中的换行符
            
            switch (choose) {
                case 1:  // 查看在线用户
                    c_msg.cmd = 3;
                    write(sockfd, &c_msg, sizeof(c_msg));
                    read(sockfd, &c_msg, sizeof(c_msg));
                    c_msg.cmd = 0;
                    online_display(c_msg);
                    break;
                    
                case 2:  // 文件传输
                    c_msg.cmd = 4;
                    filesend(sockfd, c_msg);
                    c_msg.cmd = 0;
                    break;
                    
                case 3:  // 设置在线状态
                    c_msg.cmd = 5;
                    strcpy(c_msg.name, username);
                    write(sockfd, &c_msg, sizeof(c_msg));
                    c_msg.cmd = 0;
                    printf("ok.\n");
                    break;
                    
                case 4:  // 设置隐身状态
                    c_msg.cmd = 6;
                    strcpy(c_msg.name, username);
                    write(sockfd, &c_msg, sizeof(c_msg));
                    c_msg.cmd = 0;
                    printf("ok.\n");
                    break;
            }
        } else {
            // 发送普通聊天消息
            write(sockfd, &c_msg, sizeof(c_msg));
        }
    }
}

/**文件发送函数
   向服务器发送文件*/
void filesend(int sockfd, struct Msg msg) {
    FILE *fq;
    char filename[50];
    
    printf("请输入你要打开的文件名及路径，如~/1.jpg\n");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = 0;  // 去除换行符
    
    // 打开文件
    if ((fq = fopen(filename, "rb")) == NULL) {
        printf("打开文件%s出现错误\n", filename);
        return;
    }
    
    // 分段读取并发送文件
    while (!feof(fq)) {
        msg.flen = fread(msg.file, 1, sizeof(msg.file), fq);
        write(sockfd, &msg, sizeof(msg));
    }
    
    // 发送文件结束标志
    msg.flen = -1;
    write(sockfd, &msg, sizeof(msg));
    printf("file send successful.\n");
    strcpy(msg.file, "");
    
    fclose(fq);
}

/**消息接收线程函数
   持续接收服务器消息并处理*/
void* recv_thread(void* p) {
    struct Msg c_msg;
    FILE *fp;
    int read_rec;
    
    // 初始化消息结构体
    memset(&c_msg, 0, sizeof(c_msg));
    
    while (1) {
        // 接收服务器消息
        read_rec = read(sockfd, &c_msg, sizeof(c_msg));
        
        if (read_rec <= 0) {
            // 连接断开或错误
            printf("与服务器的连接已断开\n");
            break;
        }
        
        if (c_msg.cmd == 0) {  // 普通聊天消息
            printf("\n%s:%s\n", c_msg.name, c_msg.msg);
            // 移除重复的提示输出，让主循环统一处理提示
        } else if (c_msg.cmd == 4) {  // 文件接收
            // 创建接收文件
            if ((fp = fopen("~/Desktop/recived/new.jpg", "a+")) == NULL) {
                printf("File open failed.\n");
                break;
            }
            
            printf("\nreceive a file from : %s \n", c_msg.name);
            
            // 分段接收文件内容
            while (1) {
                if (c_msg.flen == -1) {  // 文件传输结束
                    break;
                } else {
                    fwrite(c_msg.file, 1, c_msg.flen, fp);
                    read(sockfd, &c_msg, sizeof(c_msg));
                }
            }
            
            printf("receive a file -> ~/Desktop/recived\n");
            // 移除重复的提示输出，让主循环统一处理提示
            fclose(fp);
        } else if (c_msg.cmd == 3) {  // 在线用户列表
            // 这个命令由主循环处理，接收线程忽略
            continue;
        }
        
        // 重置消息结构体，避免残留数据影响
        memset(&c_msg, 0, sizeof(c_msg));
    }
    
    return NULL;
}

/**服务菜单显示函数*/
void service_menu() {
    printf("====#     MENU    #====\n");
    printf("\t1.查看user state\n");
    printf("\t2.传输文件\n");
    printf("\t3.设置在线\n");
    printf("\t4.设置隐身\n");
    printf("====#             #====\n");
}

/**在线用户显示函数*/
void online_display(struct Msg disp_msg) {
    int m = 0;
    printf("============IN THIS CHATROOM============\n");
    
    // 格式化显示在线用户列表
    for (m = 0; m < 3; m++) {
        printf(" %-10s ", disp_msg.online[m]);
    }
    
    for (m = 3; m < disp_msg.len + 3; m++) {
        if (m % 3 == 0)
            printf("\n");
        printf("  %-10s ", disp_msg.online[m]);
    }
    
    printf("\n");
    printf("========================================\n");
}

/**主函数
   程序入口点*/
int main() {
    init();   // 初始化客户端
    start();  // 启动主循环
    return 0;
}