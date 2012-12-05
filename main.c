#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <strings.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>


#define MAXCLIENTCOUNT   1024
#define MAXBUFSIZE   10240

//a simple http server
// store the client_fd and the recv buf;need send buf?
// status 0: connected ,1:read head,2:head_read_complete,3:open_file_for_write? transfer,4:transfer
struct client{
    int clifd;
    int readfd;     //store the open file discription for client
    int status;     //status ? finish the request?
    int head_len;   // the current head data len
    char head[MAXBUFSIZE];   //store the head of request
    int recv_buf_len;
    char recv_buf[MAXBUFSIZE];
    int  send_buf_len;
    char send_buf[MAXBUFSIZE];
};


// 读取http请求的头部
int read_header(int clientfd);

//解析http头部，主要是获取要请求的文件名,并打开它，返回打开的文件描述符
int parse_head(int clientfd);

int make_server_listen_socket(int port,int backlog);

int process_request(int clientfd,FD_SET *fdset);
// accept a client,malloc the struct client memory,modify the Fd_set
int accept_conect(int listenfd,FD_SET *fdset);
//read file ,wirte to the clientfd
int response(int clientfd,FD_SET *fdset);

//the client fd array,we now supported 1024 clients
int cli_fd_array[1024];
struct client * client_array[MAXCLIENTCOUNT];
// store the max read fd
int maxreadfd;
//store the max witeset fd
int maxwritefd;

int main(int argc,char **argv)
{
    int listenfd;
    if((listen_fd=make_server_listen_socket(8080,5))<0){
        fprintf(stderr,"make_server_listen socket error");
        exit(-1);
    }

    //save the maxfd;
    struct timeval timeout;
    timeout.tv_sec=2;
    timeout.tv_usec=0;
    FD_SET rst;
    FD_SET rall;
    FD_SET wst;
    FD_SET wall;

    FD_ZERO(&rall);
    FD_ZERO(&wall);

    FD_SET(listen_fd,&all);
    while(1)
    {
        rst=all;
        wst=wall;
        int ncount;
        if((ncount=select(&rst,&wst,NULL,&timeout))<0){
            perror("select error\n");
            continue;
        }
        if(ncount==0)
            continue;
        //check the read socket
        int i;
        for(i=3;i<=maxreadfd;i++)
        {
            // accept a connect
        if(FD_ISSET(i,&rst)){
            if(i==listen_fd)
                accept_conect(listen_fd,&rall);
            else 
               process_request(i,&rall);
            }
        }
        
        //check the write socket
        for(i=3;i<=maxwritefd;i++){
            if(FD_ISSET(i,&wst))
                response(i,&wall);
        }
    }


}

int make_server_listen_socket(int port,int backlog)
{
    struct sockaddr_in serveraddr;

    int listen_fd;

    if((listen_fd=socket(AF_INET,SOCK_STREAM,0))<0){
        perror("server create  socket error\n");
        return -1;
    }


    bzero(&serveraddr,sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(port);

    if(bind(listen_fd,(struct sockaddr *)&serveraddr,sizeof(serveraddr))<0){
        perror("bind error\n");
        return -2;
    }

    if(listen(listen_fd,backlog)<0){
        perror("listen error\n");
        return -3;
    }

    return listen_fd;
}
int accept_conect(int listenfd,FD_SET *fdset)
{
    int clientfd;
    clientfd=accept(listenfd);
    if(clienfd<0){
        perror("accept error\n");
        return -1;
    }

    //add the socket in readset;
    FD_SET(clientfd,fdset);
    client_array[clientfd]=(struct client *)malloc(sizeof(struct client ));
    client_array[clientfd]->clifd=clientfd;
    client_array[clientfd]->readfd=-1;
    client_array[clientfd]->recv_buf_len=0;
    client_array[clientfd]->send_buf_len=0;
    
    //change the maxfd;
    if(clientfd>maxreadfd)
        maxreadfd=clientfd;

    return clientfd;
}

int process_request(int clientfd,FD_SET *fdset)
{
    struct client* cliptr=client_array[clietnfd];

    int ret;
    if(cliptr->status==1)
       ret=read_header(clientfd);
    if(ret==0)
        response(clientfd,fdset);
    //the connect has closed,clear the fd_set
    if(ret==-2)
        FD_CLR(clientfd,fdset)
    



}

int response(int clientfd,FD_SET *fdset)
{
    struct client * cliptr=client_array[clientfd];

    //the head has_read complete
    if(cliptr->status==2)







}

int read_header(int clientfd)
{
    struct client * cliptr=client_array[clientfd];
    int nread;
    // MAXBUFSIZE-cliptr->head_len-1 make the last char  NULL
    nread=read(clientfd,cliptr->head_len,MAXBUFSIZE-cliptr->head_len-1);
    if(nread<0){
        perror("read socket %d \n",clientfd);
        return -1;    //read error;
    }
    //connect close by client;
    else  if(nread==0)
    {
        printf("connect closed %d \n",clientfd); 
       free(client_array[clientfd]);
       client_array[clientfd]=NULL;
       return -2;  //connect close by client;


    }


    //now we check the head is complete ?
    int start=cliptr->head_len-2;
    //make the start >=0
    if(start<0)
        start=0;
    int i;
    int end=head_len+nread;

    for(i=start;i<head_len+nread;++i){
        if(i<=head_len+nread-4)
        if(cliptr->head[i]=='\r' && cliptr->head[i+1]=='\n' cliptr->head[i+2]=='\r' cliptr->head[i+3]=='\n'){
            //find the head;
            break;
        }
    }
    if(i<=head_len+nread-4){
        status=2;
        return 0;
    }

}

int parse_head(int clientfd)
{






}



