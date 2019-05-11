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

int connect_server(const char *host, const char *port){
    // Create a socket client
    int client= socket( PF_INET6, SOCK_STREAM, 0);
    if (client== 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    //find address with host and the port
    struct sockaddr_in6 in6;
    memset(&in6,0,sizeof(in6));
    in6.sin6_family = AF_INET6;
    in6.sin6_port = htons(atoi(port));
    //convert to in6
    int s=inet_pton(AF_INET6,host,&in6.sin6_addr.s6_addr);
    if (s <= 0) {
        if (s == 0)
            fprintf(stderr, "Not in presentation format");
        else
            perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    //connect
    if(connect(client,(struct sockaddr *)&in6,sizeof(in6))!=0){
        perror("connect()");
        exit(EXIT_FAILURE);

    }
    return client;
 
}

//read from sock and store in buffer, stop when '\n' or nb >size
void receive_message(int sock, char *buffer, int size) {
    int nb=0;
    char* buff=buffer;
    do{
        int r=read(sock,buff,1);
        if(r<0){
            perror("read");
            exit(EXIT_FAILURE);
        }
        if(r==0){
            printf("quit!\n");
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
        printf("usage: client + host + port\n");
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
