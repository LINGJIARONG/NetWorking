#include <sys/types.h>
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include<signal.h>
#define PORT 42000


void client_arrived(int sock){
    while(42){
        //receive input from client
        char buffer[100] ={};
        ssize_t  vr = read(sock,buffer,sizeof(buffer));
        //printf("%d",(int)vr);
        if(vr<=0){
            if(vr==0){
                printf("client quit");
                fflush(stdout);
               
                break;
            }
            else{
                perror("read");
                break;
            }
        }
        buffer[vr]=0;
//        if(strncmp(buffer,"quit",4)==0)
//            break;
        printf("%s",buffer);
        //return the same string
        ssize_t wr=write(sock,buffer,strlen(buffer));
        if(wr<0){
            perror("write");
            break;
        }
    }
    if(close(sock)!=0){
        perror("close()");
        exit(EXIT_FAILURE);
    }
    
    
}

int main(int argc, char  *argv[]) { 
    int server;
    struct sockaddr_in6 in6;
    
    // Create a socket server
    server= socket( AF_INET6, SOCK_STREAM, 0);
    if (server == 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    int optval = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    signal(SIGPIPE, SIG_IGN);
    //bind
    memset(&in6,0,sizeof(in6));
    in6.sin6_family = PF_INET6;
    in6.sin6_port = htons(PORT);
    
    // attach socket to the port 42000
    int connect=bind(server, (struct sockaddr *)&in6,sizeof(in6));
    if (connect<0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server, 1) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }
    while(42){
        int client_sock= accept(server,NULL,NULL);
        if (client_sock<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        client_arrived(client_sock);
    }
    
    return 0;
} 
