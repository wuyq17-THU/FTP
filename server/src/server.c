#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "server.h"
#define MAX_SIZE 8192

int ftp_server_response(const char* sentence) {
    char buffer[MAX_SIZE];
    char tmp[100];
    char filename[100];
    char user_name[100];
    //char rename_path[100];
    //获取用户输入命令
    get_command_arg(sentence, client_cmd, client_arg);
    printf("%s %s \n", client_cmd, client_arg);
    //登录
    if(strcmp(client_cmd, "USER") == 0){
        strcpy(user_name, client_arg);
        send_infomation(client_sock, "331 Guest login ok");
        client_status = USER;
    }
    else if(strcmp(client_cmd, "PASS") == 0){
        if(check_user(user_name, client_arg) == 1){
            send_infomation(client_sock, "230 Guest login ok, access restrictions apply.");
            client_status = PASS;
        }
        else{
            send_infomation(client_sock, "501 Please check your password.");
        }
    }
    //系统以及传输数据类型
    else if(strcmp(client_cmd, "SYST") == 0){
        send_infomation(client_sock, "215 UNIX Type: L8");
        client_status = PASS;
    }
    else if(strcmp(client_cmd, "TYPE") == 0){
        send_infomation(client_sock, "200 Type set to I.");
        client_status = PASS;
    }
    //主动被动模式选择
    else if(strcmp(client_cmd, "PORT") == 0){
        get_address(file_addr, client_arg);
        file_port = get_port(client_arg);
        client_status = PORT;
        send_infomation(client_sock, "200 PORT command successful.");
    }//被动模式
    else if(strcmp(client_cmd, "PASV") == 0){
        char send_add[100];
        file_port = 20000 + rand() % 45536;
        //获取被动模式下本机IP地址
        struct hostent *ht = NULL;
        char host[128]; //主机域名
        gethostname(host, sizeof(host));
        ht = gethostbyname(host);
        for(int i=0;;i++){
            if(ht->h_addr_list[i] != NULL){
                sprintf(file_addr, "%s", inet_ntoa(*(struct in_addr*)(ht->h_addr_list[i])));
            } else{
                break;
            }
        }
        printf("file_addr IP:%s",file_addr);
        //将ip中的点换成逗号
        strcpy(send_add, file_addr);
        int len_fileIP = strlen(send_add);
        for(int i=0;i<len_fileIP;i++)
        {
            if(send_add[i] == '.')
                send_add[i] = ',';
        }
        //已经处理完本机IP地址，可以发送给client

        //被动模式下服务端新建文件传输socket
        struct sockaddr_in file_addr_in;
        //创建socket
        if((file_listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
            perror("socket");
            exit(EXIT_FAILURE);
        }
        //设置地址以及端口
        memset(&file_addr_in, 0, sizeof(struct sockaddr_in));
        file_addr_in.sin_family = AF_INET;
        file_addr_in.sin_port = htons(file_port);
        file_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
        //inet_aton(file_addr, &file_addr_in.sin_addr);
        //绑定套接字
        if (bind(file_listen_sock, (struct sockaddr*)&file_addr_in,
            sizeof(file_addr_in)) == -1) {
            printf("Error bind(): %s(%d)\n", strerror(errno), errno);
            exit(EXIT_FAILURE);
        }
        //监听端口
        if (listen(file_listen_sock, 10) == -1) {
            printf("Error listen(): %s(%d)\n", strerror(errno), errno);
            exit(EXIT_FAILURE);
        }
        //创建套接字完成

        //发送数据到client
        sprintf(buffer, "227 Entering Passive Mode (%s,", send_add);
        sprintf(tmp, "%d", file_port / 256);
        strcat(buffer, tmp);
        strcat(buffer, ",");
        sprintf(tmp, "%d", file_port % 256);
        strcat(buffer, tmp);
        strcat(buffer, ")");
        send_infomation(client_sock, buffer);
        client_status = PASV;
    }

    //todo:RETR and STOR
    //cmd RETR
    else if(strcmp(client_cmd, "RETR") == 0){
        if (client_status == PORT) {
            char info[1000];
            send_infomation(client_sock, "150 Opening BINARY mode data connection.");
            connect_server(&file_connect_sock, file_addr, file_port);
            strcpy(filename, cur_path);
            strcat(filename, client_arg);
            int file_size = get_file_size(filename);
            if(file_size < 0){
                send_infomation(client_sock, "502 File not found");
            }
            else{
                send_file(file_connect_sock, filename);
                close(file_connect_sock);
                sprintf(info, "226 Transfer complete (%d bytes).", file_size);
                transfer_size += file_size;
                send_infomation(client_sock, info);
            }
        }
        else if(client_status == PASV) {
            char info[1000];
            send_infomation(client_sock, "150 Opening BINARY mode data connection.");
            if ((file_connect_sock = accept(file_listen_sock, NULL, NULL)) == -1) {
                printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            }
            strcpy(filename, cur_path);
            strcat(filename, client_arg);
            int file_size = get_file_size(filename);
            if(file_size < 0){
                send_infomation(client_sock, "502 File not found");
            }
            else{
                send_file(file_connect_sock, filename);
                close(file_connect_sock);
                close(file_listen_sock);
                sprintf(info, "226 Transfer complete (%d bytes).", file_size);
                transfer_size += file_size;
                send_infomation(client_sock, info);
            }
        }
        else{
            send_infomation(client_sock, "425 No TCP connection was established.");
        }
        client_status = RETR;
    }
    //cmd STOR
    else if(strcmp(client_cmd, "STOR") == 0){
        if (client_status == PORT) {
            char info[1000];
            connect_server(&file_connect_sock, file_addr, file_port);
            send_infomation(client_sock, "150 Opening BINARY mode data connection.");
            strcpy(filename, cur_path);
            strcat(filename, client_arg);
            recv_file(file_connect_sock, filename);
            int file_size = get_file_size(filename);
            close(file_connect_sock);
            sprintf(info, "226 Transfer complete (%d bytes).", file_size);
            transfer_size += file_size;
            send_infomation(client_sock, info);
        }
        else if (client_status == PASV) {
            char info[1000];
            if ((file_connect_sock = accept(file_listen_sock, NULL, NULL)) == -1) {
                printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            }
            send_infomation(client_sock, "150 Opening BINARY mode data connection.");
            strcpy(filename, cur_path);
            strcat(filename, client_arg);
            recv_file(file_connect_sock, filename);
            int file_size = get_file_size(filename);
            sprintf(info, "226 Transfer complete (%d bytes).", file_size);
            close(file_connect_sock);
            close(file_listen_sock);
            transfer_size += file_size;
            send_infomation(client_sock, info);
        }
        else{
            send_infomation(client_sock, "425 No TCP connection was established.");
        }
        client_status = STOR;
    }
    //cmd MKD make dir创建目录
    else if(strcmp(client_cmd, "MKD") == 0){
        char path[100];
        char info[100];
        char control[200];
        memset(path, 0, 100);
        strcpy(path, cur_path);
        strcat(path, client_arg);
        sprintf(control, "mkdir %s", path);
        int r = system(control);
        if(r == 0){
            sprintf(info, "257 \"%s\" dictionary created.", path);
            send_infomation(client_sock, info);
        }
        else{
            sprintf(info, "550 \"%s\": file or dictionary already exists.", path);
            send_infomation(client_sock, info);
        }
    }
    //cmd CWD change working dir改变当前目录
    else if(strcmp(client_cmd, "CWD") == 0){
        char info[200];
        memset(info, 0, 200);
        if(chdir(client_arg) >= 0){
            getcwd(cur_path,1000);
            sprintf(info, "257 CWD command successful \"%s\" is current dictonary.", cur_path);
            strcat(cur_path, "/");
            send_infomation(client_sock, info);
        }
        else{
            sprintf(info, "550 \"%s\": No such file or directory.", client_arg);
            send_infomation(client_sock, info);
        }
    }
    //cmd PWD print working dir输出当前目录
    else if(strcmp(client_cmd, "PWD") == 0){
        char info[100];
        memset(info, 0, 100);
        get_pwd(client_sock, info);
        send_infomation(client_sock, info);
        client_status = PWD;
    }
    //LIST 列出当前目录下文件和文件夹
    else if(strcmp(client_cmd, "LIST") == 0){
        if (client_status == PORT) {
            send_infomation(client_sock, "150 Opening BINARY mode data connection.");
            connect_server(&file_connect_sock, file_addr, file_port);
            get_list_server(file_connect_sock);
            close(file_connect_sock);
            send_infomation(client_sock, "226 LIST all the files.");
        }
        else if(client_status == PASV) {
            send_infomation(client_sock, "150 Opening BINARY mode data connection.");
            if ((file_connect_sock = accept(file_listen_sock, NULL, NULL)) == -1) {
                printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            }
            get_list_server(file_connect_sock);
            close(file_connect_sock);
            close(file_listen_sock);
            send_infomation(client_sock, "226 LIST all the files.");
        }
        else{
            send_infomation(client_sock, "425 No TCP connection was established.");
        }
    }
    //RMD 删除目录
    else if(strcmp(client_cmd, "RMD") == 0){
        char info[100];
        char path[100];
        char control[200];
        strcpy(path, cur_path);
        strcat(path, client_arg);
        sprintf(control, "rm -r %s", path);
        int r = system(control);
        if(r == 0){
            sprintf(info, "250 \"%s\": dictionary is deleted.", path);
            send_infomation(client_sock, info);
        }
        else{
            sprintf(info, "550 \"%s\": dictionary can't be deleted or not found", path);
            send_infomation(client_sock, info);
        }
    }
    //RNFR RNTO rename from and renanme to，两个先后相连使用的命令，改变文件或目录名
    else if(strcmp(client_cmd, "RNFR") == 0){
        char info[100];
        char control[200];
        strcpy(rename_path, cur_path);
        strcat(rename_path, client_arg);
        sprintf(control, "mv %s %s", rename_path, rename_path);
        int r = system(control);
        if(r == 0){
            sprintf(info, "350 \"%s\": File exists, ready for destination name.", client_arg);
            send_infomation(client_sock, info);
        }
        else{
            sprintf(info, "550 \"%s\": no such file or dictionary.", client_arg);
            send_infomation(client_sock, info);
        }
    }
    else if(strcmp(client_cmd, "RNTO") == 0){
        char info[100];
        char control[200];
        char name_path[200];
        strcpy(name_path, cur_path);
        strcat(name_path, client_arg);
        sprintf(control, "mv %s %s", rename_path, name_path);
        int r = system(control);
        if(r == 0){
            sprintf(info, "250 File \"%s\" renamed to \"%s\".", rename_path, name_path);
            send_infomation(client_sock, info);
        }
        else{
            send_infomation(client_sock, "502 Can't rename the file or dictionary");
        }
    }
    //退出并关闭连接
    else if((strcmp(client_cmd, "QUIT") == 0) || (strcmp(client_cmd, "ABOR") == 0)){
        char info[1000];
        sprintf(info, "221 Goodbye. All the file you transfer(RETR & STOR) is %d bytes", transfer_size);
        send_infomation(client_sock, info);
        //close(client_sock);
        //close(server_sock);
        //exit(0);
        return 0;
    }
    //鲁棒性，异常输入处理
    else{
        send_infomation(client_sock, "502 Invalid command.");
    }
    return 1;
}

//todo:recv_file and send_file
void send_file(const int sock, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if(!f){
        printf("Not the File\n");
        return;
    }
    char buffer[MAX_SIZE];
    int n;

    do {
        n = fread(buffer, 1, 8190, f);
        send(sock, buffer, n, 0);
    } while (n > 0);

    fclose(f);
}

void recv_file(const int sock, const char* filename) {
    FILE* f = fopen(filename, "wb");
    int n;
    char buffer[MAX_SIZE];

    do {
        n = recv(sock, buffer, MAX_SIZE, 0);
        fwrite(buffer, 1, n, f);
    } while (n > 0);

    fclose(f);
}

int get_file_size(char *filename){
    FILE *f = fopen(filename, "r");
    if(!f){
        return -1;
    }
    fseek(f, 0L, SEEK_END);
    int file_size = ftell(f);

    return file_size;
}

int connect_server(int* client_sock, const char *server_IP, const int port) {
    struct sockaddr_in server_addr;

    if ((*client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_IP, &server_addr.sin_addr) <= 0) {
        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    if (connect(*client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Error connect(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    return 0;
}

void get_address(char* addr, const char* str){
    char* ptr;
    char tmp[100];
    strcpy(tmp, str);
    addr[0] = '\0';

    ptr = strtok(tmp, ",");
    strcat(addr, ptr);
    strcat(addr, ".");
    ptr = strtok(NULL, ",");
    strcat(addr, ptr);
    strcat(addr, ".");
    ptr = strtok(NULL, ",");
    strcat(addr, ptr);
    strcat(addr, ".");
    ptr = strtok(NULL, ",");
    strcat(addr, ptr);
    //strcat(addr, ".");
    //int len = strlen(addr);
    //addr[len-1] = '\0';
    printf("PORT addr:%s\n",addr);
    return;
}

int get_port(const char* str) {
    char* arg = NULL;
    char temp[100];
    int port = 0;
    strcpy(temp, str);
    //printf("%s\n", tmp);

    arg = strtok(temp, ",");
    arg = strtok(NULL, ",");
    arg = strtok(NULL, ",");
    arg = strtok(NULL, ",");
    arg = strtok(NULL, ",");
    port = atoi(arg) * 256;
    arg = strtok(NULL, ",");
    port += atoi(arg);

    printf("PORT port:%d\n",port);
    if(port < 1024)
        port = 2048;
    return port;
}

int check_user(char *user_name, char *user_pass){
    if(strcmp(user_name, "anonymous") == 0){
        return 1;
    }
    return 1;
}

void get_command_arg(const char* buffer, char *cmd, char *arg) {
    char temp[1000];
    memset(temp, 0, 1000);

    strcpy(temp, buffer);
    char *flag = NULL;
    //空格分割出命令
    flag = strtok(temp, " ");
    strcpy(cmd, flag);
    flag = strtok(NULL, " ");
    //分割出命令中的参数
    if(flag != NULL){
        strcpy(arg, flag);
    }
}

void ftp_server_argPort(int* port, const int argc, const char **argv) {
    int i;
    *port = 21;
    for (i = 0; i < argc; i ++) {
        if (strcmp(argv[i], "-port") == 0) {
            *port = atoi(argv[i + 1]);
            break;
        }
    }
    //printf("%d\n", *port);
}

void ftp_server_argRoot(char* root, const int argc, const char **argv) {
    int i;
    strcpy(root, "/tmp/");
    for (i = 0; i < argc; i ++) {
        if (strcmp(argv[i], "-root") == 0) {
            strcpy(root, argv[i + 1]);
            break;
        }
    }
    if (root[strlen(root) - 1] != '/')
        strcat(root, "/");
    //printf("%s\n", root);
}

void recv_data(const int sock, char* sentence) {
    char buffer[MAX_SIZE];
    unsigned int len;
    memset(buffer, 0, sizeof(buffer));
    recv(sock, buffer, MAX_SIZE, 0);
    printf("recv: /%s\n", buffer);
    len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\r' || buffer[len - 1] == '\n')) len --;
    buffer[len] = '\0';
    strcpy(sentence, buffer);
}

void send_infomation(const int sock, const char* buffer) {
    char sentence[MAX_SIZE];
    strcpy(sentence, buffer);
    strcat(sentence, "\r\n");
    send(sock, sentence, strlen(sentence), 0);
    printf("sent: /%s\n", buffer);
}

void ftp_server_loop(){
    while (1) {
        memset(sentence, 0, sizeof(sentence));
        memset(client_cmd, 0, 1000);
        memset(client_arg, 0, 1000);
        recv_data(client_sock, sentence);
        //todo
        int answer = ftp_server_response(sentence);
        if(!answer){
            break;
        }
    }
}

int main(const int argc, const char **argv) {
    pid_t pid;

    ftp_server_argPort(&server_port, argc, argv);
    ftp_server_argRoot(cur_path, argc, argv);

    //监听socket和连接socket不一样，后者用于数据传输
    struct sockaddr_in addr;
    //char sentence[8192];
    //int p;
    //int len;

    //创建socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    //设置本机的ip和port
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr("127.0.0.1");	//监听"0.0.0.0"
    printf("server IP:%s\nport:%d\n", inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));

    //将本机的ip和port与socket绑定
    if (bind(server_sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        printf("Error bind(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    //开始监听socket
    if (listen(server_sock, 10) == -1) {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    //持续监听连接请求
    while (1) {
        //等待client的连接 -- 阻塞函数
        if ((client_sock = accept(server_sock, NULL, NULL)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            break;
        }

        pid = fork();
        if (pid < 0) {
            printf("Error forking child process");
        } else if (pid == 0) {
            close(server_sock);
            client_status = CONN;
            //printf("server_sock closed:%d\n", server_sock);
            send_infomation(client_sock, "220 Anonymous FTP server ready.");
            //printf("child process:%d\n", pid);
            ftp_server_loop();
            close(client_sock);
            //printf("client_sock closed:%d\n", client_sock);
            return 0;
        }
        close(client_sock);
        //printf("main process:%d\n client_sock closed:%d\n", pid, client_sock);
    }
    close(server_sock);
    return 0;
}
    //close(listenfd);
    //return 0;
        //榨干socket传来的内容
        /*p = 0;
        while (1) {
            int n = read(connfd, sentence + p, 8191 - p);
            if (n < 0) {
                printf("Error read(): %s(%d)\n", strerror(errno), errno);
                close(connfd);
                continue;
            } else if (n == 0) {
                break;
            } else {
                p += n;
                if (sentence[p - 1] == '\n') {
                    break;
                }
            }
        }
        //socket接收到的字符串并不会添加'\0'
        sentence[p - 1] = '\0';
        len = p - 1;

        //字符串处理
        for (p = 0; p < len; p++) {
            sentence[p] = toupper(sentence[p]);
        }

        //发送字符串到socket
        p = 0;
        while (p < len) {
            int n = write(connfd, sentence + p, len + 1 - p);
            if (n < 0) {
                printf("Error write(): %s(%d)\n", strerror(errno), errno);
                return 1;
            } else {
                p += n;
            }
        }

        close(connfd);
    }*/

