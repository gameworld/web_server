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
struct client{
    int clifd;
    int readfd;     //store the open file discription for client
    int status;     //status ? finish the request?
    int recv_buf_len;
    char head[MAXBUFSIZE];   //store the head of request
    char recv_buf[MAXBUFSIZE];
    int  send_buf_len;
    char send_buf[MAXBUFSIZE];
};


// 读取http请求的头部
int read_header(int clientfd);

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

    if(cliptr->status==1)
        read_header(clientfd);

    if(read(




}

int response(int clientfd,FD_SET *fdset)
{





}

int read_header(int clientfd)
{
    struct client * cliptr=client_array[clientfd];


}


