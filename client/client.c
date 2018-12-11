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
#define host "192.168.1.157"                                          // master端的ip
#define INS 6                                                         // 脚本个数


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
char file_log[INS][20] = {"./Log/Mem.log", 
                        "./Log/Disk.log", 
                        "./Log/CPU.log", 
                        "./Log/Sys.log",
                        "./Log/User.log",
                         "./Log/MPD.log"};
char script_name[][50] = {"bash ../shell/MemLog.sh 20", 
                           "bash ../shell/DiskLog.sh", 
                           "bash ../shell/CPULog.sh", 
                           "bash ../shell/SysInfo.sh",
                           "bash ../shell/Users.sh",
                           "bash ../shell/MPD.sh"};
int sleep_time[INS] = {5, 5, 5, 5, 5, 5};

void *func(void *argv) {
    int i = *((int *)argv);
    char command[50] = {0};
    sprintf(command, "%s >> %s\0", script_name[i], file_log[i]);
    //printf("%d : %s\n", i, command);
    while(1) {
        pthread_mutex_lock(&mut[i]);
        system(command);
        pthread_mutex_unlock(&mut[i]);
        sleep(sleep_time[i]);
    }
}

void run_shell() {
    pthread_t t[INS + 1];
    for (int i = 0; i < INS; i++) {
        num[i] = i;
        if (pthread_create(&t[i], NULL, func, (void *)&num[i]) == -1) {
            printf("pthread_create is error\n");
            exit(1);
        }
        sleep(1);
    }
}

int main () {
    run_shell();                                                      // 开6个线程, 运行6个脚本
    int sock_client;
    struct sockaddr_in dest_addr; 
    if ((sock_client = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket");
        return -1;
    }
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    dest_addr.sin_addr.s_addr = inet_addr(host);
    if (connect(sock_client, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Connect");
        close(sock_client);
        exit(0);
    } else printf ("Connect success!\n");
    close(sock_client);

    int client_listen = create_listen(PORT1);
    while (1) {
        struct sockaddr_in server_addr;
        socklen_t len = sizeof(server_addr);
        int socketfd;
        if ((socketfd = accept(client_listen, (struct sockaddr *)&server_addr, &len)) < 0) {
            perror("accept failed!");
	        continue;
        }
        // 传文件
	    close(socketfd);
    }
    close (client_listen);
    return 0;
}
