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

int main () {
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
	    close(socketfd);
    }
    close (client_listen);
    return 0;
}
