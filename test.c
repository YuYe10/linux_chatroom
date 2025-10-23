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
#include "serverLogin.h"

int main() {
    char* db_pwd = getenv("CHATROOM_DB_PASS");
    if (db_pwd == NULL) {
        fprintf(stderr, "Database password environment variable not set\n");
        return 1;
    }
    printf("Database password: %s\n", db_pwd);
    return 0;
}