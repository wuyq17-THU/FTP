//
// Created by 吴寅秋 on 2020/10/28.
//
#ifndef MY_CLIENT_CLIENT_H
#define MY_CLIENT_CLIENT_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#define MAX_SIZE 8192
#define NONE -1
#define CONN 0
#define USER 1
#define PASS 2
#define PORT 3
#define PASV 4
#define RETR 5
#define STOR 6
#define SYST 7
#define TYPE 8
#define QUIT 9
#define ABOR 10
#define PWD 11
#define CWD 12
#define CDUP 13
#define MKD 14
#define DELE 15
#define RMD 16
#define RNFR 17
#define RNTO 18


int server_sock, client_sock;
int server_port;
int file_port;
int file_listen_sock, file_connect_sock;
int client_status;

char cur_path[1000];
char sentence[MAX_SIZE];
char file_addr[100];
char client_cmd[1000];
char client_arg[1000];
char server_cmd[1000];
char server_arg[1000];

int client_loop();
void recv_data(const int sock, char* sentence);
void recv_response(char* reply);
void send_file(const int sock, const char* filename);
void recv_file(const int sock, const char* filename);
//建立服务器socket
int create_socket(int* server_sock, int port);
//连接服务器
int connect_server(int* client_sock, const char *server_IP, const int port);
//发送buffer
void send_infomation(const int sock, const char* buffer);
void get_list_client(const int sock_data);
void get_address(char* addr, const char* str);
int get_port(const char* str);
void get_command_arg(const char* buffer, char *cmd, char *arg);
void get_address_port_client(const char* buffer, char *arg);

#endif //MY_CLIENT_CLIENT_H
