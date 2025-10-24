/**
 * @file database.h
 * @brief 数据库操作模块头文件
 * @details 定义数据库相关的数据结构、函数声明和常量
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <mysql/mysql.h>
#include <stdio.h>

// 用户信息结构体
struct user {
    char username[20];  // 用户名
    char userpwd[20];   // 用户密码
};

// 在线用户信息结构体
struct onlineuser {
    char username[20];   // 用户名
    char userstate[20];  // 用户状态（online/offline）
};

/**
 * @brief 初始化数据库连接
 * @param con MySQL连接指针
 * @return 初始化后的MySQL连接指针
 */
MYSQL * db_initial(MYSQL *con);

/**
 * @brief 连接到MySQL数据库
 * @param con MySQL连接指针
 * @param hostip 数据库服务器IP地址
 * @param username 数据库用户名
 * @param pwd 数据库密码
 * @param db 数据库名称
 * @return 连接成功后的MySQL连接指针
 */
MYSQL * db_connect(MYSQL *con ,char *hostip, char *username , char * pwd, char *db);

/**
 * @brief 创建数据库
 * @param con MySQL连接指针
 * @param cmd 创建数据库的SQL命令
 */
void db_create(MYSQL *con,char *cmd);

/**
 * @brief 创建数据表
 * @param con MySQL连接指针
 * @param db 数据库名称
 * @param cmd 创建表的SQL命令
 */
void table_create(MYSQL *con,char *db,char *cmd);

/**
 * @brief 显示数据表内容
 * @param con MySQL连接指针
 * @param query 查询SQL语句
 * @param display 存储显示结果的二维数组
 * @return 表中数据项的总数
 */
int table_display(MYSQL *con,char *query,char display[100][20]);

/**
 * @brief 检查用户名是否已存在
 * @param con MySQL连接指针
 * @param name 要检查的用户名
 * @return 1-存在，0-不存在
 */
int data_exist(MYSQL *con,char *name);

/**
 * @brief 验证用户名和密码是否正确
 * @param con MySQL连接指针
 * @param name 用户名
 * @param pwd 密码
 * @return 1-正确，0-错误
 */
int data_judge(MYSQL *con,char *name,char *pwd);

/**
 * @brief 插入用户数据
 * @param con MySQL连接指针
 * @param name 用户名
 * @param pwd 密码
 * @return 1-成功，0-失败
 */
int data_insert(MYSQL *con,char *name,char *pwd);

/**
 * @brief 插入在线用户数据
 * @param con MySQL连接指针
 * @param name 用户名
 * @param state 用户状态
 * @return 1-成功，0-失败
 */
int data_insert_online(MYSQL *con,char *name,char *state);

/**
 * @brief 删除用户数据
 * @param con MySQL连接指针
 * @param name 要删除的用户名
 * @return 1-成功，0-失败
 */
int data_delete(MYSQL *con,char *name);

/**
 * @brief 删除在线用户数据
 * @param con MySQL连接指针
 * @param name 要删除的用户名
 * @return 1-成功，0-失败
 */
int data_delete_online(MYSQL *con,char *name);

#endif