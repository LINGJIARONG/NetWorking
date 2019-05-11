#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include <netdb.h>
#include<signal.h>

#define PORT 42000
#define NCLIENTS 100

//type  client 
typedef struct client_data_s {
    int used;   //being usd ->1 else->0
    int sock;
    pthread_t thread;
}client_data;
//client data
client_data client[NCLIENTS];
int nr_clients;
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
//find unused id
int unused_id(){
    int i;
    for(i=0;i<NCLIENTS;i++ ){
        if(!client[i].used)
            return i;
    }
    return -1;
}
//use an usused id
int alloc_client(){
    pthread_mutex_lock(&mutex);
    int i=unused_id();
    if(i==-1){
        return -1 ;
    }
    client[i].used=1;
    pthread_mutex_unlock(&mutex);
    return i;
}
//set free an id
void free_client(int client_id){
    pthread_mutex_lock(&mutex);
    client[client_id].used=0;
    pthread_mutex_unlock(&mutex);
}

void* client_main(void* arg){
    int id =(int)arg;
    int sock=client[id].sock;
    while(42){
        
        //receive input from client
        char buffer[100] ={};
        ssize_t  vr = read(sock,buffer,sizeof(buffer));
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
//        if(strncmp(buffer,"quit",4)==0)//client type quit: close session
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
    free_client(id);
    
}

void client_arrived(int sock){
    int id=alloc_client();
    //no more places
    if(id==-1){
        printf("server too busy");
        close(sock);
        
    }
    client[id].sock=sock;
    pthread_t aClient;
    int re=pthread_create(&aClient, NULL,client_main, (void *)id);
    if(re!=0){
        printf("error: can't create thread");
        close(sock);
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
        struct sockaddr_in6 addr;
        socklen_t addr_len = sizeof(addr);
        int client_sock = accept(server ,(struct sockaddr *)&addr, &addr_len);
        char buf[NCLIENTS];
        if(getnameinfo((struct sockaddr *)&addr, addr_len, buf, sizeof(buf), NULL,
                       0, 0) == 0)
            printf("connection from %s\n", buf);
        if (client_sock<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        client_arrived(client_sock);
    }
    
    return 0;
} 
