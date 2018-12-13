/*************************************************************************
	> File Name: master.c
	> Author: gpx
	> Mail: 1457495424@qq.com
	> Created Time: 2018年11月15日 星期四 18时37分46秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "./master.h"

#define INS 5
#define PORT 33333                                             // 建立的监听端口
#define MAX_SIZE 1024                                          // 每次接收文件的最长字节长度

void *func(void *);

int queue[INS + 1] = {0};                                      // 存储每个链表有几个节点
pthread_mutex_t mut[INS + 1];                                  // 用来给每个链表加锁

typedef struct Node {
    struct sockaddr_in addr;
    struct Node *next;
}Node, *LinkedList;

LinkedList linkedlist[INS + 1];

typedef struct Mypara{
    struct sockaddr_in addr;
    int num;
}Mypara;

// 初始化5个链表, 有一个没用的头结点
void init_linkedlist(int n) {
    for (int i = 0; i < n; i++) {
        linkedlist[i] = (LinkedList)malloc(sizeof(Node));
        linkedlist[i]->next = NULL;
    }
    return ;
}

// 在链表head的头部插入节点node
void insert(LinkedList head, Node *node) {
    node->next = head->next;
    head->next = node;
    return ;
}

// 输出链表head的所有节点
void output(LinkedList head, int num) {
    if (head->next == NULL) {
        printf("linkedlist %d is empty\n", num);
        return ;
    }
    printf("linkedlist %d has %d node\n", num, queue[num]);
    Node *p = head->next;
    char logfile[20];
    while (p) {
        printf("%s:%d\n", inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
        p = p->next;
    }
    printf("\n");
    return ;
}

// 找到链表长度最短的链表
int find_min(int N, int *arr) {
    int min = arr[0];
    int ans = 0;
    for (int i = 1; i < N; i++) {
        if (min > arr[i]) {
            min = arr[i];
            ans = i;
        }
    }
    return ans;
}

// 从配置文件pathname中读取信息key_name, 存在字符串value中
int get_conf_value(char *pathname, char *key_name, char *value) {
    FILE *fp = NULL;
    if ((fp = fopen (pathname, "r")) == NULL) {
        printf ("pathname NULL!\n");
        exit(0);
    }
    size_t len = 0;
    char *line = NULL;
    ssize_t  read;
    int key_len = strlen(key_name);
    while (( read = getline(&line, &len, fp)) != -1) {
        if (strstr(line, key_name) != NULL) {
            if (line[key_len] == '=') {
                strncpy(value, &line[key_len + 1], read - key_len - 1);
                key_len = strlen(value);
                value[key_len - 1] = '\0';
                fclose(fp);
                return 1;
            }
        }
    } 
    fclose(fp);
    return 0;
}

// 创建端口号为port的监听
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
        close(server_listen);
		return -1;
	}
    if (listen(server_listen, 20) < 0) {
        perror("listen error");
        exit(0);
    }
    return server_listen;
}

// 判断这个客户端是否已经加在了链表中
int exist(struct sockaddr_in addr) {
    for (int i = 0; i < INS; i++) {
        Node *p = linkedlist[i]->next;
        while (p) {
            if (p->addr.sin_addr.s_addr == addr.sin_addr.s_addr) {
                return 1;
            }
            p = p->next;
        }
    }
    return 0;
}

// 删除节点
void delete_node(LinkedList head, Node *del, int pid) {
    Node *p, *q;
    p = head;
    q = head->next;
    while (q) {
        if (q->addr.sin_addr.s_addr == del->addr.sin_addr.s_addr) {
            printf("delete %s:%d\n", inet_ntoa(q->addr.sin_addr), ntohs(q->addr.sin_port));
            p->next = q->next;
            free(q);
            queue[pid]--;
            break;
        }
        p = p->next;
        q = q->next;
    }
    return ;
}

// 连接客户端
int connect_client(Node *p) {
    int sockfd;
    struct sockaddr_in addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        close(sockfd);
        perror("socket failed\n");
        return 0;
    }
    char port[10] = {0};
    get_conf_value("./config.conf", "port", port);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    addr.sin_addr.s_addr = p->addr.sin_addr.s_addr;
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return 0;
    }
    //close(sockfd);
    return sockfd;
}

char file_log[6][20] = {"Mem.log", "Disk.log", "CPU.log", "Sys.log", "User.log", "MPD.log"};

// 收文件
void recv_file(int sockfd, char *str, Node *p) {
    int recode;
    while (1) {
        if (recv(sockfd, &recode, 4, 0) <= 0) {
            break ;
        }
        printf("receive recode %d\n", recode);
        int server_temp = connect_client(p);
        if (server_temp == 0) {
            perror("connect");
            //close(server_temp);
            continue ;
        }
        char buffer[MAX_SIZE];
        char addr[80];
        sprintf(addr, "%s/%s", str, file_log[recode - 100]);
        printf("start receive file %s\n", addr);
        FILE *file = fopen(addr, "a+");
        if (file == NULL) {
            fclose(file);
            continue ;
        }
        int a;
        while ((a = recv(server_temp, buffer, MAX_SIZE - 1, 0)) > 0) {
            fprintf(file, "%s", buffer + 1);
            memset(buffer, 0, sizeof(buffer));
        }
        printf("finish receive file %s\n", addr);
        fclose(file);
        close(server_temp);
    }
    return ;
}


// 遍历链表, 判断是否可以连接到客户端, 连接不到的删除
void connect_or_delete(LinkedList head, int pid) {
    if (head->next == NULL) {
        //printf("NULL %d\n", pid);
        return ;
    }
    Node *p, *temp;
    p = head->next;
    int sockfd;
    while (p) {
        //printf("%s\n", inet_ntoa(p->addr.sin_addr));
        if ((sockfd = connect_client(p)) == 0) {
            //printf("connect error\n");
            pthread_mutex_lock(&mut[pid]);
            temp = p->next;
            delete_node(head, p, pid);
            p = temp;
            pthread_mutex_unlock(&mut[pid]);
        } else {
            char str[50] = {"./Log/"};
            char ip[50];
            strcpy(ip, inet_ntoa(p->addr.sin_addr));
            strcat(str, ip);
            if (opendir(str) == NULL) {
                mkdir(str, 0755);
            }
            pthread_mutex_lock(&mut[pid]);
            recv_file(sockfd, str, p);                               // 收文件
            p = p->next;
            pthread_mutex_unlock(&mut[pid]);
        }
        close(sockfd);
    }
    return ; 
}

int main() {
    //freopen("in.in", "r", stdin);
    init_linkedlist(INS);
    pthread_t t[INS + 1];
    Mypara para[INS + 1];
    char value[20] = {0}, start[10] = {0}, finish[10] = {0}, port[10] = {0};
    get_conf_value("./config.conf", "prename", value);
    get_conf_value("./config.conf", "start", start);
    get_conf_value("./config.conf", "finish", finish);
    get_conf_value("./config.conf", "port", port);
    for (int i = atoi(start); i <= atoi(finish); i++) {
        char ip[100];
        sprintf(ip, "%s.%d", value, i);
        struct sockaddr_in addr;
        addr.sin_port = htons(atoi(port));
        addr.sin_addr.s_addr = inet_addr(ip);
        int sub = find_min(INS, queue);
        Node *p;
        p = (Node *)malloc(sizeof(Node));
        p->addr = addr;
        p->next = NULL;
        insert(linkedlist[sub], p);
        queue[sub]++;
    }
    /*
    for (int i = 0; i < INS; i++) {
        printf("%d ", queue[i]);
        printf("   ......\n");
        output(linkedlist[i], i);
    }
    */
    for (int i = 0; i < INS; i++) {
        para[i].num = i;
        if (pthread_create(&t[i], NULL, func, (void *)&para[i]) == -1) {
            printf("error\n");
            exit(1);
        }
    }
    int server_listen = create_listen(PORT);
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int socketfd;
        if ((socketfd = accept(server_listen, (struct sockaddr*)&client_addr, &len)) < 0) {
            perror("accept error");
            continue;
        }
        if (exist(client_addr)) {
            printf("already exists\n");
            continue;
        }
        int sub = find_min(INS, queue);
        Node *p;
        p = (Node *)malloc(sizeof(Node));
        p->addr = client_addr;
        p->next = NULL;
        pthread_mutex_lock(&mut[sub]);
        insert(linkedlist[sub], p);
        queue[sub]++;
        pthread_mutex_unlock(&mut[sub]);
        printf("insert into %d linkedlist\n", sub);
        output(linkedlist[sub], sub);
        close(socketfd);
    }
    //close(server_listen);
    /*
    pthread_join(t[0], NULL);
    pthread_join(t[1], NULL);
    pthread_join(t[2], NULL);
    pthread_join(t[3], NULL);
    pthread_join(t[4], NULL);
    */
    return 0;
}

void *func(void *argv) {
    Mypara *para;
    para = (Mypara *)argv;
    while (1) {
        connect_or_delete(linkedlist[para->num], para->num);
        //printf("id = %d\n", para->num);
        sleep(2);
    }
    return NULL;
}
