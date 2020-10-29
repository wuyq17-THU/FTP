//
// Created by 吴寅秋 on 2020/10/27.
//

#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

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
int file_listen_sock, file_connect_sock;
int file_port;
int client_status;
int transfer_size = 0;

char file_addr[100];
char sentence[MAX_SIZE];
char cur_path[1000];
char client_cmd[1000];
char client_arg[1000];
char server_cmd[1000];
char server_arg[1000];

int ftp_server_response(const char* sentence);
void ftp_server_loop();
void get_pwd(int socket, char *info);
void get_list_server(const int sock);
void handle_address_to_send(char *send, char *address);
void ftp_server_argPort(int* port, const int argc, const char **argv);
void ftp_server_argRoot(char* root, const int argc, const char **argv);
void recv_data(const int sock, char* sentence);
void send_file(const int sock, const char* filename);
void recv_file(const int sock, const char* filename);
int check_user(char *user_name, char *user_pass);
//建立服务器socket
int create_socket(int* my_sock, int port);
//连接服务器
int connect_server(int* client_sock, const char *server_IP, const int port);
//发送buffer
void send_infomation(const int sock, const char* buffer);
int change_path_server(char *path);
int get_file_size(char *filename);
void get_address(char* addr, const char* str);
void trimstr(char *str, int n);
int get_port(const char* str);
void get_IP_server(char *IP);
void get_command_arg(const char* buffer, char *cmd, char *arg);
void get_address_port_server(const char* buffer, char *arg);

#endif //SERVER_SERVER_H
