#include "client.h"

void recv_data(const int sock, char* sentence) {
    char buffer[MAX_SIZE];
    int len;
    memset(buffer, 0, sizeof(buffer));
    recv(sock, buffer, MAX_SIZE, 0);
    len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\r' || buffer[len - 1] == '\n')) len --;
    buffer[len] = '\0';
    strcpy(sentence, buffer);
}
void recv_response(char* reply) {
    char sentence[8192];
    memset(sentence, 0, sizeof(sentence));
    recv_data(client_sock, sentence);
    printf("%s\n", sentence);
    strcpy(reply, sentence);
}
void send_file(const int sock, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if(!f){
        printf("Not the File\n");
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

void get_list_client(const int sock){
    int n;
    char buffer[MAX_SIZE];

    do {
        n = recv(sock, buffer, MAX_SIZE, 0);
        fwrite(buffer, 1, n, stdout);
    } while (n > 0);
}
int create_socket(int* server_sock, int port) {
    struct sockaddr_in addr;

    if ((*server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(*server_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        printf("Error bind(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    if (listen(*server_sock, 10) == -1) {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    return 0;
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

    //printf("suc\n");

    return 0;
}
void send_infomation(const int sock, const char* buffer) {
    char sentence[MAX_SIZE];
    strcpy(sentence, buffer);
    strcat(sentence, "\r\n");
    send(sock, sentence, strlen(sentence), 0);
}
void get_address(char* addr, const char* str) {
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

    if(port < 1024)
        port = 2048;
    return port;
}
void get_command_arg(const char* buffer, char *cmd, char *arg) {
    if(buffer == NULL)
        return;
    char temp[1000];
    memset(temp, 0, 1000);

    strcpy(temp, buffer);
    char *flag = NULL;

    flag = strtok(temp, " ");
    strcpy(cmd, flag);
    flag = strtok(NULL, " ");

    if(flag != NULL){
        strcpy(arg, flag);
    }
}
void get_address_port_client(const char* buffer, char *arg){
    char temp[1000];
    memset(temp, 0, 1000);

    strcpy(temp, buffer);
    char *flag = NULL;

    flag = strtok(temp, " ");
    flag = strtok(NULL, " ");
    flag = strtok(NULL, " ");
    flag = strtok(NULL, " ");
    flag = strtok(NULL, " ");
    flag++;
    strcpy(arg, flag);
    arg[strlen(arg) - 1] = '\0';
}

int client_loop() {
    int len;
    char sentence[MAX_SIZE];
    char reply[MAX_SIZE];
    for(int p = 0; p < 100000000; p++);
    printf("ftp client> ");
    fgets(sentence, 4096, stdin);
    len = strlen(sentence);

    while (len > 0 && (sentence[len - 1] == '\n' || sentence[len - 1] == '\r'))
        len --;
    sentence[len] = '\0';

    get_command_arg(sentence, client_cmd, client_arg);

    if(strcmp(client_cmd, "USER") == 0){
        client_status = USER;
        send_infomation(client_sock, sentence);
        recv_response(reply);
    }
    else if(strcmp(client_cmd, "PASS") == 0){
        client_status = PASS;
        send_infomation(client_sock, sentence);
        recv_response(reply);
    }
    else if(strcmp(client_cmd, "TYPE") == 0){
        client_status = TYPE;
        send_infomation(client_sock, sentence);
        recv_response(reply);
    }
    else if(strcmp(client_cmd, "MKD") == 0){
        client_status = MKD;
        send_infomation(client_sock, sentence);
        recv_response(reply);
    }
    else if(strcmp(client_cmd, "DELE") == 0){
        client_status = DELE;
        send_infomation(client_sock, sentence);
        recv_response(reply);
    }
    else if(strcmp(client_cmd, "RMD") == 0){
        client_status = RMD;
        send_infomation(client_sock, sentence);
        recv_response(reply);
    }
    else if(strcmp(client_cmd, "RNFR") == 0){
        client_status = RNFR;
        send_infomation(client_sock, sentence);
        recv_response(reply);
    }
    else if(strcmp(client_cmd, "Rq") == 0){
        client_status = RNTO;
        send_infomation(client_sock, sentence);
        recv_response(reply);
    }
    else if(strcmp(client_cmd, "PWD") == 0){
        client_status = PWD;
        send_infomation(client_sock, sentence);
        recv_response(reply);
    }
    else if(strcmp(client_cmd, "CWD") == 0){
        client_status = CWD;
        send_infomation(client_sock, sentence);
        recv_response(reply);
    }
    else if(strcmp(client_cmd, "CDUP") == 0){
        client_status = CDUP;
        send_infomation(client_sock, sentence);
        recv_response(reply);
    }
    else if(strcmp(client_cmd, "SYST") == 0){
        client_status = SYST;
        send_infomation(client_sock, sentence);
        recv_response(reply);
    }
    else if(strcmp(client_cmd, "PORT") == 0){
        create_socket(&file_listen_sock, get_port(client_arg));
        send_infomation(client_sock, sentence);
        recv_response(reply);
        client_status = PORT;
    }
    else if(strcmp(client_cmd, "PASV") == 0){
        send_infomation(client_sock, sentence);
        recv_response(reply);
        get_address_port_client(reply, server_arg);
        get_address(file_addr, server_arg);
        file_port = get_port(server_arg);
        client_status = PASV;
    }
    else if(strcmp(client_cmd, "RETR") == 0){
        if (client_status == PORT) {
            send_infomation(client_sock, sentence);
            if ((file_connect_sock = accept(file_listen_sock, NULL, NULL)) == -1) {
                printf("Error accept(): %s(%d)\n", strerror(errno), errno);
                return 1;
            }
            recv_response(reply);
            recv_file(file_connect_sock, client_arg);
            close(file_connect_sock);
            close(file_listen_sock);
            recv_response(reply);
        }
        else if(client_status == PASV) {
            send_infomation(client_sock, sentence);
            connect_server(&file_connect_sock, file_addr, file_port);
            recv_response(reply);
            recv_file(file_connect_sock, client_arg);
            close(file_connect_sock);
            recv_response(reply);
        }
        else {
            send_infomation(client_sock, sentence);
            recv_response(reply);
        }
        client_status = RETR;
    }
    else if(strcmp(client_cmd, "LIST") == 0){
        if (client_status == PORT) {
            send_infomation(client_sock, sentence);
            if ((file_connect_sock = accept(file_listen_sock, NULL, NULL)) == -1) {
                printf("Error accept(): %s(%d)\n", strerror(errno), errno);
                return 1;
            }
            recv_response(reply);
            get_list_client(file_connect_sock);
            close(file_connect_sock);
            close(file_listen_sock);
            recv_response(reply);
        }
        else if(client_status == PASV) {
            send_infomation(client_sock, sentence);
            connect_server(&file_connect_sock, file_addr, file_port);
            recv_response(reply);
            get_list_client(file_connect_sock);
            close(file_connect_sock);
            recv_response(reply);
        }
        else {
            send_infomation(client_sock, sentence);
            recv_response(reply);
        }
    }
    else if(strcmp(client_cmd, "QUIT") == 0){
        send_infomation(client_sock, sentence);
        recv_response(reply);
        close(client_sock);
        exit(0);
    }
    else if(strcmp(client_cmd, "ABOR") == 0){
        send_infomation(client_sock, sentence);
        recv_response(reply);
        close(client_sock);
        exit(0);
    }
    else if(strcmp(client_cmd, "STOR") == 0){
        if (client_status == PORT) {
            send_infomation(client_sock, sentence);
            if ((file_connect_sock = accept(file_listen_sock, NULL, NULL)) == -1) {
                printf("Error accept(): %s(%d)\n", strerror(errno), errno);
                return 1;
            }
            recv_response(reply);
            send_file(file_connect_sock, client_arg);
            close(file_connect_sock);
            close(file_listen_sock);
            recv_response(reply);
        }
        else if(client_status == PASV) {
            send_infomation(client_sock, sentence);
            connect_server(&file_connect_sock, file_addr, file_port);
            recv_response(reply);
            send_file(file_connect_sock, client_arg);
            close(file_connect_sock);
            recv_response(reply);
        }
        else {
            send_infomation(client_sock, sentence);
            recv_response(reply);
        }
        client_status = STOR;
    }
    else{
        send_infomation(client_sock, sentence);
        recv_response(reply);
    }

    return 0;
}

int main(int argc, char **argv) {
    char reply[MAX_SIZE];
    connect_server(&client_sock, argv[1], atoi(argv[2]));
    recv_response(reply);
    client_status = CONN;
    while (client_loop() == 0);
    close(client_sock);
    return 0;
}
