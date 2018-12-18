/*************************************************************************
	> File Name: client.c
	> Author: gpx
	> Mail: 1457495424@qq.com
	> Created Time: 2018年11月25日 星期日 09时23分13秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "./client.h"
#define PORT 33333                                                    // master端开的端口
#define PORT1 9999                                                    // client端开的端口
#define PORT2 33334                                                   // master端开的接收警报信息的端口
#define host "192.168.1.158"                                          // master端的ip
#define INS 6                                                         // 脚本个数
#define MAX_SIZE 1024                                                 // 单次发送文件的最大字节数

int create_listen(int port) {
    int server_listen;
    struct sockaddr_in my_addr;
    if ((server_listen = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("socket error");
        exit(1);
    }
    my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //bzero(&(my_addr.sin_zero), sizeof(my_addr));
    if (bind(server_listen, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) < 0) {
		perror("bind error");
        //close(server_listen);
		exit(0);
	}
    if (listen(server_listen, 20) < 0) {
        perror("listen error");
        exit(0);
    }
    return server_listen;
}

pthread_mutex_t mut[INS + 1];                                         // 用于给6个脚本得出来的结果文件加锁
int num[INS + 1];                                                     // num[i]代表线程的编号
char file_log[INS][20] = {"./Log/Mem.log", "./Log/Disk.log", "./Log/CPU.log", 
                        "./Log/Sys.log", "./Log/User.log", "./Log/MPD.log"};
char script_name[][50] = {"bash ../shell/MemLog.sh 20", "bash ../shell/DiskLog.sh", "bash ../shell/CPULog.sh", 
                           "bash ../shell/SysInfo.sh", "bash ../shell/Users.sh", "bash ../shell/MPD.sh"};
int sleep_time[INS] = {5, 5, 5, 5, 5, 5};

// 检测是否有报警信息
void warning_detecte(int num, char *buffer) {
    if (num == 4) {
        if (strstr(buffer, "warning") == NULL) return ;
    } else if (num == 5) {
        if (!buffer[0]) return ;
    }
    int warning_sock;
    struct sockaddr_in warning_addr;
    if ((warning_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket");
        close(warning_sock);
        return ;
    }
    warning_addr.sin_family=  AF_INET;
    warning_addr.sin_port = htons(PORT2);
    warning_addr.sin_addr.s_addr = inet_addr(host);
    if (connect(warning_sock, (struct sockaddr *)&warning_addr, sizeof(warning_addr)) < 0) {
        perror("socket");
        sleep(10);
        close(warning_sock);
        return ;
    } else {
        send(warning_sock, &num, 4, 0);
        send(warning_sock, buffer, strlen(buffer), 0);
        close(warning_sock);
    }
    return ;
}

void *func(void *argv) {
    int i = *((int *)argv), parameter = 0;
    char buffer[MAX_SIZE] = {0};
    while(1) {
        printf("pthread %d run shell %s\n", i, script_name[i]);
        FILE *fp;
        if ((fp = popen(script_name[i], "r")) == NULL) {
            perror("fail to popen");
            break ;
        }
        pthread_mutex_lock(&mut[i]);
        FILE *fp1 = fopen(file_log[i], "a+");
        if (fread(buffer, 1, MAX_SIZE, fp)) {
            if (i == 3 || i == 5) {
                warning_detecte(i, buffer);
            }
            fwrite(buffer, 1, strlen(buffer), fp1);
            memset(buffer, 0, MAX_SIZE);
        }
        fclose(fp1);
        fclose(fp);
        pthread_mutex_unlock(&mut[i]);
        sleep(sleep_time[i]);
    }
}

// 开6个线程, 运行6个脚本, 并且将6个脚本的运行结果存储到6个.log文件中
void run_shell() {
    printf("Run shell started!\n");
    pthread_t t[INS + 1];
    for (int i = 0; i < INS; i++) {
        num[i] = i;
        if (pthread_create(&t[i], NULL, func, (void *)&num[i]) == -1) {
            printf("pthread_create is error\n");
            exit(1);
        }
    }
}

// 将6个.log文件传给master端
void send_file(int socketfd, int client_listen) {
    for (int i = 100; i < 100 + INS; i++) {
        printf("start send recode %d\n", i);
        if (send(socketfd, &i, 4, 0) < 0) {
            perror("send");
            continue ;
        }
        printf("finish send recode %d\n", i);
        struct sockaddr_in server_short;
        socklen_t len = sizeof(server_short);
        int client_short;
        if ((client_short = accept(client_listen, (struct sockaddr *)&server_short, &len)) < 0) {
            close(client_short);
            continue ;
        }
        FILE *file;
        char buffer[MAX_SIZE];
        pthread_mutex_lock(&mut[i - 100]);
        if ((file = fopen(file_log[i - 100], "rb")) == NULL) {
            perror("fileopen error");
            pthread_mutex_unlock(&mut[i - 100]);
            continue ;
        }
        printf("start send file %s\n", file_log[i - 100]);
        while (!feof(file)) {
            fread(buffer, sizeof(char), MAX_SIZE - 1, file);
            send(client_short, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer));
        }
        printf("finish send file %s\n", file_log[i - 100]);
        fclose(file);
        char command[50] = {0};
        printf("start clear file %s\n", file_log[i - 100]);
        sprintf(command, "%s > %s\0", "", file_log[i - 100]);
        system(command);
        printf("finish clear file %s\n", file_log[i - 100]);
        pthread_mutex_unlock(&mut[i - 100]);
        close(client_short);
    }
    return ;
}

// 心跳, 告诉master端自己上线了
void *heart(void *argv) {
    while (1) {
        int sock_client;
        struct sockaddr_in dest_addr;
        if ((sock_client = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Socket");
            close(sock_client);
        }
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        dest_addr.sin_addr.s_addr = inet_addr(host);
        if (connect(sock_client, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("Connect");
            close(sock_client);
            return NULL;
        } else printf ("Connect success!\n");
        close(sock_client);
        sleep(5);
    }
}
int main () {
    pthread_t t;
    if (pthread_create(&t, NULL, heart, NULL) == -1) {
        printf("pthread_create is error\n");
        exit(1);
    }
    run_shell();                                                      // 开6个线程, 运行6个脚本
    int client_listen = create_listen(PORT1);
    while (1) {
        struct sockaddr_in server_addr;
        socklen_t len = sizeof(server_addr);
        int socketfd;
        if ((socketfd = accept(client_listen, (struct sockaddr *)&server_addr, &len)) < 0) {
            perror("accept failed!");
	        continue;
        }
        send_file(socketfd, client_listen);                           // 将6个.log文件传给master端
	    close(socketfd);
    }
    close(client_listen);
    return 0;
}
