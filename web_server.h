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
#include <sys/epoll.h>
#include <sys/time.h>
#include <pthread.h>


#define MAXCLIENTCOUNT   1024
#define MAXBUFSIZE   10240
#define MAXPATHNAME  1024
#define MAXHEADLINE  20
