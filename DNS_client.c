#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include<string.h>
#include<netinet/in.h>
#include <netdb.h>
//connect to server and send
/*
int connect_server(const char *host, const char *port){
    // Create a socket client
    int client= socket( AF_INET6, SOCK_STREAM, 0);
    if (client== 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    //find address with host and the port
    struct sockaddr_in6 in6;
    memset(&in6,0,sizeof(in6));
    in6.sin6_family = PF_INET6;
    in6.sin6_port = htons(atoi(port));
    //convert to in6
    inet_pton(AF_INET6,host,&in6.sin6_addr.s6_addr);
    //connect
    connect(client,(struct sockaddr *)&in6,sizeof(in6));
    return client;
 
}*/
//DNS
int connect_server(const char *hostname, const char *port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_V4MAPPED | AI_ALL;
    struct addrinfo *res;
    int rc = getaddrinfo(hostname, port, &hints, &res);
    if(rc < 0) {
        printf("invalid address/port: %s\n", gai_strerror(rc));
        exit(1);
    }for(; res != NULL; res = res->ai_next) {
        int srv_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(srv_sock < 0)
            continue;
        if(connect(srv_sock, res->ai_addr, res->ai_addrlen) < 0) {
            perror("connection failed");
            close(srv_sock);
            continue;
        }return srv_sock;
    }exit(0);
}

//read from sock and store in buffer, stop when '\n' or nb >size
void receive_message(int sock, char *buffer, int size) {
    int nb=0;
    char* buff=buffer;
    do{
        if(read(sock,buff,1)<0){
            perror("read");
            exit(EXIT_FAILURE);
        }
        nb++;
        buff++;
       
    }while(buffer[nb-1]!=10);
    //printf("%d",nb);

    buffer[nb]='\0';
}


//scan a message, send to server, print the result
void speak_to_server(int sock){
    while(1){
        printf(">>>");
        char buf[100]={};
        char buffer[100]={};
        memset(buf,'\0',100);
        memset(buffer,'\0',100);
        
        if(fgets(buf,100,stdin)==NULL){
            perror("fgets()");
            exit(EXIT_FAILURE);
        }
        //  printf("%sfrom client\n %lu",buf,strlen(buf));
        if(write(sock,buf,strlen(buf))<0){
            perror("write");
            exit(EXIT_FAILURE);
        }
        receive_message(sock,buffer,100);
        printf("%s",buffer);
    }
}


int main(int argc, char *argv[]){
    if(argc!=3){
        printf("usage: clent + host + port");
        exit(EXIT_FAILURE);
    }
    int client=connect_server(argv[1],argv[2]);
    speak_to_server(client);
    if(close(client)!=0){
        perror("close()");
        exit(EXIT_FAILURE);
    }
    return 0;
    
}
