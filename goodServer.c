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
#include <string.h>

#define PORT 42000
#define NCLIENTS 100
#define MESSAGE_MAXLEN 1024
#define MESSAGE_MAXNUM 10
//type  client 
typedef struct client_data_s {
    int used;   //being usd ->1 else->0
    int sock;
    char nick[256];
    char message[MESSAGE_MAXNUM][MESSAGE_MAXLEN];
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

//use an usused id,mark it to used
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

//set free an id,mark it unused
void free_client(int client_id){
    pthread_mutex_lock(&mutex);
    client[client_id].used=0;
    for(int i=0;i<MESSAGE_MAXNUM;i++)
        memset(client[client_id].message[i],'\0',MESSAGE_MAXLEN);
    memset(client[client_id].nick,'\0',256);
    pthread_mutex_unlock(&mutex);
}


//read from sock and store in buffer, stop when '\n' or nb >size
int receive_message(int sock, char *buffer, int size) {
    int nb=0;
    char* buff=buffer;
    do{
        int r=read(sock,buff,1);
        if(r<0){
            perror("read");
            return r;
        }
        else if(r==0){
            printf("deconnextion");
            return 0;
        }
        nb++;
        if(nb>=size-1)
            break;
        buff++;
    }while(buffer[nb-1]!=10);
    buffer[nb]='\0';
    printf("%s",buffer);
    return nb;
}

//echo
int do_echo(int client_id,char* args,char* resp, int resp_len){
    snprintf(resp, resp_len, "ok %s", args);
    return 1;
}

//random number
int do_rand(int client_id,char* args,char* resp, int resp_len){
    int val = strtol(args, NULL, 10); //string to number, max number
    srand(time(NULL));
    int j=(int)(rand()%val);//j= 0~val-1
    snprintf(resp, resp_len, "ok %d\n", j);
    return 1;
}

int alphabtOrNumber(char o){
    if((o<='z'&&o>='a')||(o>='A'&&o<='Z')||(o>='0'&&o<='9'))
        return 1;
    return 0;
}
//nickname
int do_nick(int client_id,char* args,char* resp, int resp_len){
    
    int arg_len = strlen(args);
//    printf("%d\n",arg_len);
//    fflush(stdout);
    for(int i=0;i<arg_len-2;i++){
        if(!alphabtOrNumber(args[i])){
            snprintf(resp, resp_len, "fail: nickname unavailable only numbers and alphabets(%c)\n",args[i]);
            return 1;
        }
    }
    
    args[arg_len-2]='\0';
    
    
    for(int i=0;i<NCLIENTS;i++){
        if(client[i].used==1&&i!=client_id){
            if(strcmp(client[i].nick,args)==0){
                snprintf(resp, resp_len, "fail: nickname unavailable: already used\n");
//                printf("%s\n%s",client[i].nick,args);
//                fflush(stdout);
                return 1;
            }
        }
    }
//    printf("%d\n%s",arg_len-2,args);
//    fflush(stdout);
    strcpy(client[client_id].nick,args);
    snprintf(resp, resp_len, "ok, Welcome : %s\n",args);
    return 1;
}
//cmd==list
int do_list(int client_id,char* args,char* resp, int resp_len){
    char ret[2000]="";
    int cursor=0;
    for(int i=0;i<NCLIENTS;i++){
        if(client[i].used==1&&i!=client_id){
            int len=strlen(client[i].nick);
            strcpy(ret+cursor,client[i].nick);
//            printf("copy source:%s",client[i].nick);
//            printf("copy destination:%s",ret);
            
            cursor+=len;
            ret[cursor]=' ';
            cursor++;
        }
    }
    snprintf(resp, resp_len, "ok, %s\n",ret);
    return 1;
}

//Send
int do_send(int client_id,char* args,char* resp, int resp_len){
    if(client[client_id].nick[0]=='\0'){
        snprintf(resp, resp_len, "pls log in first : nick + your name  \n");
        return 1;
    }
    char *nick;
    char from[256]="";
    strcpy(from,client[client_id].nick);
    nick=strsep(&args," ");
    if(nick==NULL||args==NULL){
        snprintf(resp, resp_len, "usage: send + nickname + message(not empty) \n");
        return 1;
    }
    int empty=0;
    for(int i=0;i<NCLIENTS;i++){
        if(client[i].used==1&&i!=client_id){
            if(strcmp(client[i].nick,nick)==0){
                for(empty=0;empty<MESSAGE_MAXNUM;empty++){
                    if(client[i].message[empty][0]=='\0'){
                        strcpy(client[i].message[empty],"from ");
                        int len=strlen(from);
//                        printf("len_from%d,%s",len,from);
//                        fflush(stdout);
                        from[len]=' ';
                        from[len+1]=':';
                        from[len+2]=' ';
                        strcpy(client[i].message[empty]+5,from);
                        len=strlen(from);
                        strcpy(client[i].message[empty]+len+5,args);
                        snprintf(resp, resp_len, "ok, message send to %s : %s",nick,args);
                        return 1;
                    }
                }
                if(empty==MESSAGE_MAXNUM){
                    snprintf(resp, resp_len, "not availavle: messenger of %s always full of memory \n",nick);
                    return 1;
                }
            }
        }
    }
    snprintf(resp, resp_len, "not availavle: %s not found \n",nick);
    return 1;
}

//Recv
int do_recv(int client_id,char* args,char* resp, int resp_len){
    if(client[client_id].nick[0]=='\0'){
        snprintf(resp, resp_len, "pls log in first : nick + your name  \n");
        return 1;
    }
    char *nick;
    int e;
    for(e=MESSAGE_MAXNUM-1;e>=0;e--){
        if(client[client_id].message[e][0]!='\0'){
            snprintf(resp, resp_len, "ok, message received %s",client[client_id].message[e]);
            memset(client[client_id].message[e],'\0',MESSAGE_MAXLEN);
            return 1;
        }
    }
    snprintf(resp, resp_len, "Messager mail empty! \n");
    return 1;

}


//quit: close cilent connexion

int do_quit(int client_id,char* args,char* resp, int resp_len){
    snprintf(resp, resp_len, "[server disconnected]\n");
    int sock=client[client_id].sock;
    if(write(sock, resp, strlen(resp)) < 0) {
        perror("could not send message");
    }
    return -1;//negatif numver-> quit
}

//try to understand command
int eval_msg(int client_id, char *msg, char *resp, int resp_len) {
    char *cmd, *args;
    cmd=strsep(&msg," ");
    int len=strlen(cmd);
    args=msg;
    if(args==NULL)
        cmd[len-1]='\0';
//    printf("cmd:%s,len_cmd %d,args:%s\n",cmd,len,args);
//    fflush(stdout);
    if(strcmp(cmd, "echo") == 0)
        return do_echo(client_id, args, resp, resp_len);
    else if(strcmp(cmd, "rand") == 0)
        return do_rand(client_id, args, resp, resp_len);
    else if(strncmp(cmd, "quit",4) == 0)
        return do_quit(client_id, args, resp, resp_len);
    else if(strcmp(cmd, "nick") == 0)
        return do_nick(client_id, args, resp, resp_len);
    else if(strncmp(cmd, "list",4) == 0)
        return do_list(client_id, args, resp, resp_len);
    else if(strcmp(cmd, "send") == 0){
        return do_send(client_id, args, resp, resp_len);
        
    }
    else if(strncmp(cmd, "recv",4) == 0)
        return do_recv(client_id, args, resp, resp_len);
    else {
        int len=strlen(cmd);//if cmd has already '\n', can't give it more
        snprintf(resp, resp_len, (cmd[len-1]!='\n')?"fail unknown command %s\n":"fail unknown command %s", cmd);
        return 0;
    }
}

void* client_main(void* arg){
    int client_id =(int)arg;
    int sock=client[client_id].sock;
    
    char cmd[100] ={};//command
    char response[100] ={};//response
    while(1) {
        if(receive_message(sock, cmd, sizeof(cmd)) < 0)
            break; /* erreur: on deconnecte le client */
        if(eval_msg(client_id, cmd, response, sizeof(response)) < 0)
            break; /* erreur: on deconnecte le client */
        if(write(sock, response, strlen(response)) < 0) {
            perror("could not send message");
            break; /* erreur: on deconnecte le client */
        }
    }
    
    if(close(sock)!=0){
        perror("close()");
        exit(EXIT_FAILURE);
    }
    free_client(client_id);
    
}

void client_arrived(int sock){
    int id=alloc_client();
    //no more places,deconnexion
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
    if(setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))!=0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
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
        
        int client_sock = accept(server ,NULL,NULL);
        if (client_sock<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        client_arrived(client_sock);
    }
    
    return 0;
} 
