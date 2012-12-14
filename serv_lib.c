
#include "web_server.h"

#define MAXCMDLEN  512


//根据传入的字符串 打开文件，如果是静态网页直接打开文件，如果是动态执行的脚本，使用管道打开它
int open_pipe_or_file(char *filename,int *status);
int checkfile_type(char *filename);
extern char * workdir ;

int open_pipe_or_file(char *filename,int *status)
{
	//save the open fd
	int fd=-1;
	int filetype;
	char  cmd[MAXCMDLEN];
	char path[MAXPATHNAME];
	FILE *fp;
	//make the test that the file is exist
	if((fd=open(filename,O_RDONLY ))<0){
		strcpy(path,workdir);
		strncat(path,"404.html",MAXPATHNAME);
		fd=open(path,O_RDONLY);
		printf("404 %d\n",fd);
		return fd;
		//the page not found just return ;
	}
	//now the page is found, detemine the file type
	filetype=checkfile_type(filename);
	//static page file
	if(filetype<=1)
		return fd;
	//dynamic page file
	else if(filetype==2){
		strcpy(cmd,"python ");
		strncat(cmd,filename,MAXCMDLEN);
		fp=popen(cmd,"r");
		//open succ
		if(fp!=NULL){
			fd=fileno(fp);
		}
	}
	return fd;
}

//return the file's type ,1: static file, 2:dynamic file
int checkfile_type(char * filename)
{
	if(strstr(filename,"html")!=NULL)
		return 1;
	else if(strstr(filename,"py")!=NULL)
		return 2;
	return 0;
}

