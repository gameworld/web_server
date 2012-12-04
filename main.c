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
//a simple http server
// store the client_fd and the recv buf;need send buf?
struct client{
    int fd;
    int recv_buf_len;
    char recv_buf[10240];
    int send_buf_len;
    char send_buf[10240];
};


int make_server_listen_socket(int port,int backlog);
int process_request(int clientfd,FD_SET *fdset);
int accept_conect(int listenfd,FD_SET *fdset);

//the client fd array,we now supported 1024 clients
int cli_fd_array[1024];
struct client * client_array[MAXCLIENTCOUNT];


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
    int maxfd;
    FD_SET rst;
    FD_SET all;
    FD_SET wst;

    FD_ZERO(&all);
    FD_ZERO(&rst);

    FD_SET(listen_fd,&all);
    while(1)
    {
        rst=all;
        int ncount;
        if((ncount=select(&rst,NULL,NULL,&timeout))<0){
            perror("select error\n");
            continue;
        }
        if(ncount==0)
            continue;
        int i;
        for(i=0;i<=maxfd;i++)
        {
            // accept a connect
        if(FD_ISSET(i,&rst)){
            if(i==listen_fd)
                accept_conect(listen_fd,&all);
            process_request(i,&all);
            }
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
int process_request(int clientfd)
{

}

