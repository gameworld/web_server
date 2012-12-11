#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>


#define MAXCLIENTCOUNT   1024
#define MAXBUFSIZE   10240
#define MAXPATHNAME  1024
#define MAXHEADLINE  20

//a simple http server
// store the client_fd and the recv buf;need send buf?
// status 0: connected ,1:read head,2:head_read_complete,3:parse head ,4:open_file_for_write? transfer,5:transfer
struct client{
    int clifd;
    int readfd;     //store the open file discription for client
    int status;     //status ? finish the request?
    int httpstatus;
    int header_len;   // the current head data len
    char header[MAXBUFSIZE];   //store the head of request
    char * header_argv[MAXHEADLINE];   //parse the header,store the pointer in header
    int  recv_buf_len;
    char recv_buf[MAXBUFSIZE];
    int  send_buf_len;
    char send_buf[MAXBUFSIZE];
};


// 读取http请求的头部
int read_header(int clientfd);

//解析http头部，将头部分成argv数组
int parse_head(int clientfd);
//打开客户端请求的文件用于发送给客户端
int open_file(int clientfd);

//添加一个头部信息,及向写入的头部缓存写一行键值对 
int add_header(int clientfd,char *item);

int end_header(int clientfd);

//发送文件函数 ，新建一个线程用于文件发送,
void *send_files(void *arg);

int make_server_listen_socket(int port,int backlog);

// 清空指定的客户端的数据
int  destroy(int clientfd); 

int  process_request(int clientfd,fd_set  *fdset);
// accept a client,malloc the struct client memory,modify the Fd_set
int  accept_conect(int listenfd,fd_set  *fdset);
//read file ,wirte to the clientfd
int response(int clientfd,fd_set  *fdset);

//向客户端回复 的头部信息
int  sendheader(int clientfd,int argc,char **argv);

//the client fd array,we now supported 1024 clients
int cli_fd_array[1024];
struct client * client_array[MAXCLIENTCOUNT];
// store the max read fd
int maxreadfd;
//store the max witeset fd
int maxwritefd;

int main(int argc,char **argv)
{
    int listen_fd;
    if((listen_fd=make_server_listen_socket(8080,5))<0){
        fprintf(stderr,"make_server_listen socket error");
        exit(-1);
    }

    //save the maxfd;
    struct timeval timeout;
    timeout.tv_sec=3;
    timeout.tv_usec=0;
    fd_set rst;
    fd_set rall;
    fd_set wst;
    fd_set wall;

    FD_ZERO(&rall);
    FD_ZERO(&wall);

    FD_SET(listen_fd,&rall);
    maxreadfd=listen_fd;
    while(1)
    {
        timeout.tv_sec=5;
        timeout.tv_usec=0;
        rst=rall;
        //wst=wall;
        int ncount;
        //注意:每次select后 timeout结构体都必须重写
        if((ncount=select(maxreadfd+1,&rst,NULL,NULL,NULL))<0){
            perror("select error\n");
            continue;
        }
        static int mcount=0;
        mcount++;
        if(ncount==0){
            printf("%ld\n",timeout.tv_sec);
            continue;
        }
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

    int err;
    if(setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&err,sizeof(int))<0){
        perror("set socket option error\n");
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
int accept_conect(int listenfd,fd_set  *fdset)
{
    int clientfd;
    clientfd=accept(listenfd,NULL,0);
    if(clientfd<0){
        perror("accept error\n");
        return -1;
    }

    printf("accept %d \n",clientfd);

    //add the socket in readset;
    FD_SET(clientfd,fdset);
    client_array[clientfd]=(struct client *)malloc(sizeof(struct client ));
    client_array[clientfd]->clifd=clientfd;
    client_array[clientfd]->readfd=-1;
    client_array[clientfd]->recv_buf_len=0;
    client_array[clientfd]->send_buf_len=0;
    client_array[clientfd]->status=1;
    client_array[clientfd]->send_buf[0]='\0';
    
    //change the maxfd;
    if(clientfd>maxreadfd)
        maxreadfd=clientfd;

    return clientfd;
}

int process_request(int clientfd,fd_set *fdset)
{
    printf("process request");
    struct client* cliptr=client_array[clientfd];

    int ret;
    switch(cliptr->status)
    {
        case 1:ret=read_header(clientfd);
               if(ret==0)
                   response(clientfd,fdset);
               if(ret==-2)
                   FD_CLR(clientfd,fdset);
               break;
        case 2:response(clientfd,fdset);
               break;

    }



}

int response(int clientfd,fd_set *fdset)
{
    struct client * cliptr=client_array[clientfd];
    FD_CLR(clientfd,fdset);


    parse_head(clientfd);
    open_file(clientfd);

    int err;
    pthread_t ntid;
    pthread_attr_t attr;
    
    err=pthread_attr_init(&attr);
    if(err!=0){
        perror("pthread_attr_init error\n");
        return -1;
    }

    //创建一个线程发送文件
    err=pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    long arr[3];
    arr[0]=cliptr->readfd;
    arr[1]=clientfd;
    arr[2]=( long )fdset;
    if(err==0)
        err=pthread_create(&ntid,NULL,send_files,arr);
    pthread_attr_destroy(&attr);
    return (err);
}



int read_header(int clientfd)
{
    printf("read header\n");
    struct client * cliptr=client_array[clientfd];
    int nread;
    // MAXBUFSIZE-cliptr->head_len-1 make the last char  NULL
    nread=read(clientfd,cliptr->header+cliptr->header_len,MAXBUFSIZE-cliptr->header_len-1);
    if(nread<0){
        perror("read socket ");
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
    int start=cliptr->header_len-2;
    //make the start >=0
    if(start<0)
        start=0;
    int i;
    int end=cliptr->header_len+nread;

    for(i=start;i<cliptr->header_len+nread;++i){
        if(i<=cliptr->header_len+nread-4)
        if(cliptr->header[i]=='\r' && cliptr->header[i+1]=='\n' && cliptr->header[i+2]=='\r' && cliptr->header[i+3]=='\n'){
            //find the head;
            cliptr->status=2;
            cliptr->header_len=i+4;
            return 0;
            
        }
    }

    cliptr->header_len =cliptr->header_len + nread;
    i++;
    return 1;
}

int parse_head(int clientfd)
{
    printf("parse_header\n");
    struct client  * cliptr=client_array[clientfd];
    if(cliptr->status!=2)
        return -1;

    char url[MAXBUFSIZE];
    char filename[MAXPATHNAME];
    cliptr->header[MAXBUFSIZE-1]='\0';
    char *tempstr=cliptr->header;
    char **argv=cliptr->header_argv;
    char *p;
    int i=0;

    while((p=strstr(tempstr,"\r\n"))!=NULL  && i<MAXHEADLINE){
        *p='\0';
        argv[i]=tempstr;
        tempstr+=2;
        i++;
    }
    argv[i]=NULL;

    i=0;
    //make a test
    cliptr->status=3;
    while(argv[i]!=NULL){
        printf("%s",argv[i]);
        ++i;
    }
}

int open_file(int clientfd)
{
    struct client *cliptr=client_array[clientfd];

    if(cliptr->status!=3)
        return -1;
    char url[MAXPATHNAME];
    char pathname[MAXPATHNAME]; 
    int fd=-1;
    if(sscanf(cliptr->header_argv[0],"GET %s HTTP/1.1",url)==1){
        char *end=strstr(url,"?");
        if(end!=NULL)
        *end='\0';
        if(strcmp(url,"/")==0)
            sprintf(pathname,"./index.html");
        else
            sprintf(pathname,"./%s",url);
        if((fd=open(pathname,O_RDONLY))<0){
            fprintf(stderr,"open file %s error ",pathname);
            perror("");
            cliptr->httpstatus=404;
            if((fd=open("404.html",O_RDONLY))<0){
                fprintf(stderr,"open file  404 erro");
                perror("");
            }
        }
        else
            cliptr->httpstatus=200;
    }
    cliptr->readfd=fd;
    cliptr->status=4;
    return 0;
}


int  destroy(int clientfd) 
{
    close(clientfd);
    free(client_array[clientfd]);
    client_array[clientfd]=NULL;

}

void *send_files(void *arg)
{
     long *iarg=(long *)arg;
     struct stat statinfo;
     char buf[1024];
     char str[32];
     int readfd=iarg[0];
     int clientfd=iarg[1];

     struct client * cliptr=client_array[clientfd];


     
    switch(cliptr->httpstatus){
        case 200: strcpy(str,"OK");break;
        case 400: strcpy(str,"NOT FOUND");break;
        default: strcpy(str,"UNKNOWN");
    }
     
     sprintf(buf,"HTTP/1.1 %d %s",cliptr->httpstatus,str);
     add_header(clientfd,buf);

     if(readfd>0){
         if(fstat(readfd,&statinfo)==0){
             time_t mtime;
             time(&mtime);
             snprintf(buf,1024,"Date:%s GMT",asctime(gmtime(&mtime)));
             add_header(clientfd,buf);
             add_header(clientfd,"Server:tlw/0.1");
             add_header(clientfd,"Cache-Control:no-cache");
             snprintf(buf,1024,"Content-Length:%lu",statinfo.st_size);
             add_header(clientfd,buf);
         }
     }
     

     add_header(clientfd,"Content-Type:text/html");
     add_header(clientfd,"Connection: close");
     end_header(clientfd);

     if(write(clientfd,cliptr->send_buf,cliptr->send_buf_len)!=cliptr->send_buf_len)
         perror("send header error");
     int nread=0;
     if(readfd>0){
     while((nread=read(readfd,buf,sizeof(buf)))>0){
         if(write(clientfd,buf,nread)!=nread)
             fprintf(stderr,"write error");
             }
     close(readfd);
     }

     destroy(clientfd);
}

/*
int  sendheader(int clientfd,int argc,char **argv)
{
    char buf[1024];
    struct client * cliptr=client_array[clientfd];
    int i=0;
    int size=0;
   // int nremain=MAXBUFSIZE;
    while(i<argc){
        cliptr->send_buf_len += snprintf(buf,"%s\r\n",argv[i],);
        strncat(cliptr->send_buf,buf,MAXBUFSIZE);
    }
    //;strncat(cliptr->send_buf,"\r\n");
    //if(write(clientfd,cliptr->send_buf,cliptr->send_buf_len)!=cliptr->send_buf_len){
    //    perror("write error\n");
    //    return -1;
   // }
    return 0;
}
*/

int add_header(int clientfd,char *item)
{

    struct client * cliptr=client_array[clientfd];
    //char buf[1024];

    int strl=strlen(item);
    // 剩余的5个字节用于存放 \r\n\r\n\0;
    if(cliptr->send_buf_len+strl+5<=MAXBUFSIZE){
    strcat(cliptr->send_buf,item);
    strcat(cliptr->send_buf,"\r\n");
    cliptr->send_buf_len+=strl+2;
    }

}

int end_header(int clientfd)
{
    struct client *cliptr=client_array[clientfd];

    cliptr->send_buf_len+=2;
    strcat(cliptr->send_buf,"\r\n");

}

