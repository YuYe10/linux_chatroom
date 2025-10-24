/**
 * @file database.c
 * @brief 数据库操作模块实现文件
 * @details 实现MySQL数据库的连接、查询、插入、删除等操作
 */

#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"

/**
 * @brief 初始化MySQL数据库
 * @param con MySQL连接指针
 * @return 初始化后的MySQL连接指针
 * @note 调用mysql_library_init()初始化MySQL客户端库
 */
MYSQL * db_initial(MYSQL *con)
{
    // 初始化MySQL客户端库
    if(mysql_library_init(0,NULL,NULL)){
        fprintf(stderr, "Could't initialize MYSQL library.\n");
        return con;
    }
    // 初始化MySQL连接句柄
    con = mysql_init(NULL);
    return con;
}

/**
 * @brief 连接到MySQL数据库服务器
 * @param con MySQL连接指针
 * @param hostip 数据库服务器IP地址
 * @param username 数据库用户名
 * @param pwd 数据库密码
 * @param db 数据库名称
 * @return 连接成功后的MySQL连接指针
 * @note 使用mysql_real_connect()建立实际连接
 */
MYSQL * db_connect(MYSQL *con ,char *hostip, char *username , char * pwd, char *db)
{
    // 建立数据库连接，端口号为3306
    if(!(mysql_real_connect(con, hostip, username, pwd, db,3306,NULL,0))){
        fprintf(stderr, "\nError:%s [%d]\n", mysql_error(con),mysql_errno(con));
        exit(1);
    }
    printf("Connection Successful!\n");
    return con;
}

/**
 * @brief 创建数据库
 * @param con MySQL连接指针
 * @param cmd 创建数据库的SQL命令
 */
void db_create(MYSQL *con,char *cmd)
{
    // 执行创建数据库的SQL命令
    if(mysql_query(con,cmd)){
        printf("Create database failed.\n");
    }else{
        printf("Create database successful.\n");
    }
}

/**
 * @brief 创建数据表
 * @param con MySQL连接指针
 * @param db 数据库名称
 * @param cmd 创建表的SQL命令
 */
void table_create(MYSQL *con,char *db,char *cmd)
{
    // 选择指定的数据库
    if( mysql_select_db(con,db)){
        printf("select DB failed.\n");
    }
    // 执行创建表的SQL命令
    if(mysql_query(con,cmd)){
        printf("Create table failed.\n");
    }else{
        printf("Create table successful.\n");
    }
}

/**
 * @brief 显示数据表内容
 * @param con MySQL连接指针
 * @param query 查询SQL语句
 * @param display 存储显示结果的二维数组
 * @return 表中数据项的总数（行数×列数）
 */
int table_display(MYSQL *con,char *query,char display[100][20])
{
    MYSQL_RES *mysql_res;    // 查询结果集
    MYSQL_ROW mysql_row;     // 结果集行数据
    MYSQL_FIELD *field;      // 字段信息
    int i,j;
    int num_row,num_col;     // 行数和列数
    int count = 0;           // 数据项计数
    
    // 执行SQL查询
    if((mysql_real_query(con,query,(unsigned int)strlen(query))) != 0){
        printf("Query failed.\n");
    }
    
    // 存储查询结果
    if((mysql_res = mysql_store_result(con)) == NULL){
        printf("Store result failed.\n");
    }

    // 获取结果集的行数和列数
    num_row = mysql_num_rows(mysql_res);
    num_col = mysql_num_fields(mysql_res);
    
    printf("===========================================\n");
    int k = 0;
    
    // 遍历结果集的每一行
    for(i = 0; i <num_row ; i++){
        if((mysql_row = mysql_fetch_row(mysql_res)) == NULL){
            break;
        }
        
        // 第一行显示字段名
        if (i == 0) {
            while(field = mysql_fetch_field(mysql_res)) {
                printf(" %-10s ", field->name);
                strcpy(display[k],field->name);  // 保存字段名到显示数组
                k++;
            }
            printf("\n");
        }
        
        // 获取行数据长度信息
        mysql_fetch_lengths(mysql_res);
        
        // 遍历当前行的每一列
        for(j = 0; j <num_col ; j++){
            printf("  %-10s ", mysql_row[j] ? mysql_row[j] : "NULL" );
            // 保存数据到显示数组
            strcpy(display[3*i+j+3],mysql_row[j] ? mysql_row[j] : "NULL");
        }
        printf("\n");
    }
    
    count = num_row * num_col;  // 计算总数据项数
    printf("===========================================\n");
    
    // 释放结果集内存
    mysql_free_result(mysql_res);
    return count;
}

/**
 * @brief 检查用户名是否已存在
 * @param con MySQL连接指针
 * @param name 要检查的用户名
 * @return 1-存在，0-不存在
 */
int data_exist(MYSQL *con,char *name)
{
    struct user user1;
    char query[1024];
    strcpy(user1.username,name);

    MYSQL_RES *result;
    MYSQL_ROW row;
    MYSQL_FIELD *field;
    int num_fields;
    
    // 构建查询SQL：查找指定用户名的记录
    sprintf(query,"SELECT * FROM usrinfo WHERE username = '%s'",
        user1.username);
    
    mysql_query(con,query);
    result = mysql_store_result(con);
    num_fields = mysql_num_fields(result);
    
    // 遍历查询结果
    while ((row = mysql_fetch_row(result)))
    {
        if(field = mysql_fetch_field(result)) {
            printf("%s ,", row[1]);
            // 比较用户名是否匹配
            if(!strcmp(user1.username,row[1])){
                return 1;  // 用户名存在
            }
        }
    }
    
    mysql_free_result(result);
    mysql_commit(con);
    return 0;  // 用户名不存在
}

/**
 * @brief 验证用户名和密码是否正确
 * @param con MySQL连接指针
 * @param name 用户名
 * @param pwd 密码
 * @return 1-正确，0-错误
 */
int data_judge(MYSQL *con,char *name,char *pwd)
{
    struct user user1;
    char query[1024];
    strcpy(user1.username,name);
    strcpy(user1.userpwd,pwd);

    MYSQL_RES *result;
    MYSQL_ROW row;
    MYSQL_FIELD *field;
    int num_fields;
    int i;
    
    // 构建查询SQL：查找指定用户名的记录
    sprintf(query,"SELECT * FROM usrinfo WHERE username = '%s'",
        user1.username);
    
    mysql_query(con,query);
    result = mysql_store_result(con);
    num_fields = mysql_num_fields(result);
    
    // 遍历查询结果
    while ((row = mysql_fetch_row(result)))
    {
        if(field = mysql_fetch_field(result)) {
            printf("username:%s,", row[1]);
            printf("pwd:%s,", row[2]);
            // 验证用户名和密码是否匹配
            if(!strcmp(user1.username,row[1]) && !strcmp(user1.userpwd,row[2])){
                printf("login successful!\n");
                return 1;  // 验证成功
            }
        }
    }

    mysql_free_result(result);
    mysql_commit(con);
    return 0;  // 验证失败
}

/**
 * @brief 插入用户数据
 * @param con MySQL连接指针
 * @param name 用户名
 * @param pwd 密码
 * @return 1-成功，0-失败
 */
int data_insert(MYSQL *con,char *name,char *pwd)
{
    struct user user1;
    char query[1024];
    MYSQL_RES *result;
    strcpy(user1.username,name);
    strcpy(user1.userpwd,pwd);
    
    // 检查用户名是否已存在
    if(data_exist(con,name)){
        printf("this name has existed.\n");
        return 0;
    }
    
    // 检查用户名和密码是否正确（用于验证）
    if(data_judge(con,name,pwd)){
        printf("name/pwd wrong.\n");
        return 0;
    }
    
    // 构建插入SQL：向usrinfo表插入新用户
    sprintf(query,"insert into usrinfo(userName,userPWD) values('%s','%s')",
        user1.username ,user1.userpwd);
    
    // 执行插入操作
    if((mysql_real_query(con,query,(unsigned int)strlen(query))) != 0){
        mysql_rollback(con);  // 插入失败，回滚事务
        printf("insert failed.\n");
        return 0;
    }
    
    mysql_commit(con);  // 提交事务
    return 1;  // 插入成功
}

/**
 * @brief 插入在线用户数据
 * @param con MySQL连接指针
 * @param name 用户名
 * @param state 用户状态
 * @return 1-成功，0-失败
 */
int data_insert_online(MYSQL *con,char *name,char *state)
{
    struct onlineuser user1;
    char query[1024];
    strcpy(user1.username,name);
    strcpy(user1.userstate,state);

    // 构建插入SQL：向onlineinfo表插入在线用户信息
    sprintf(query,"insert into onlineinfo(userName,userState) values('%s','%s')",
        user1.username ,user1.userstate);
    
    // 执行插入操作
    if((mysql_real_query(con,query,(unsigned int)strlen(query))) != 0){
        mysql_rollback(con);  // 插入失败，回滚事务
        printf("insert failed.\n");
        return 0;
    }
    
    mysql_commit(con);  // 提交事务
    return 1;  // 插入成功
}

/**
 * @brief 删除用户数据
 * @param con MySQL连接指针
 * @param name 要删除的用户名
 * @return 1-成功，0-失败
 */
int data_delete(MYSQL *con,char *name)
{
    char query[1024];
    
    // 构建删除SQL：从usrinfo表删除指定用户
    sprintf(query,"delete from usrinfo where userName = '%s'", name );
    
    // 执行删除操作
    if((mysql_real_query(con,query,(unsigned int)strlen(query))) != 0){
        mysql_rollback(con);  // 删除失败，回滚事务
        printf("delete failed.\n");
        return 0;
    }
    
    mysql_commit(con);  // 提交事务
    
    // 检查是否成功删除了记录
    if( mysql_affected_rows(con)>0){
        printf("delete successful.\n");
    }
    return 1;
}

/**
 * @brief 删除在线用户数据
 * @param con MySQL连接指针
 * @param name 要删除的用户名
 * @return 1-成功，0-失败
 */
int data_delete_online(MYSQL *con,char *name)
{
    char query[1024];
    
    // 构建删除SQL：从onlineinfo表删除指定在线用户
    sprintf(query,"delete from onlineinfo where userName = '%s'", name );
    
    // 执行删除操作
    if((mysql_real_query(con,query,(unsigned int)strlen(query))) != 0){
        mysql_rollback(con);  // 删除失败，回滚事务
        printf("delete failed.\n");
        return 0;
    }
    
    mysql_commit(con);  // 提交事务
    
    // 检查是否成功删除了记录
    if( mysql_affected_rows(con)>0){
        printf("delete successful.\n");
    }
    return 1;
}