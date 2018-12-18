#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

//static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void *doit(void *vargp);
int parse_uri(char *uri, char *host, char *path, int *port);
void with_server(char t_line[], char newbuf[], char *host, int port, int connfd);

int main(int argc, char **argv){
 
  char* host[MAXLINE];
  int port;		//for client
  int listenfd;
  int *connfd; 
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  if (argc!=2){
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
  }

  // Aspect of a server : accept the request from a server
  signal(SIGPIPE,SIG_IGN);
  
  listenfd=Open_listenfd(argv[1]);		//argv[1] is a port number		
  //printf("listen fd\n");
  while(1){
	connfd=Malloc(sizeof(int));
	clientlen=sizeof(clientaddr);
	*connfd=Accept(listenfd,(SA *)&clientaddr, &clientlen);
	//printf("accept\n");

	//Getnameinfo((SA *)&clientaddr, clientlen, host, MAXLINE,(int *) port, MAXLINE,0);
	//printf("Accept connection from  (%s, %d)\n", host, port);
	Pthread_create(&tid,NULL,doit,connfd);
  }

	
    return 0;
}

void *doit(void *vargp){

  int fd=*((int *)(vargp));
  Pthread_detach(pthread_self());		//new thread gets indep
  free(vargp);

  char buf[MAXBUF], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  rio_t rio;
  int urires;
  char host[MAXLINE], path[MAXLINE];
  int port=80;

  Rio_readinitb(&rio,fd);
  Rio_readlineb(&rio,buf,MAXBUF);

  sscanf(buf, "%s %s %s", method, uri, version); 	//parse request line
  if (strcasecmp(method,"GET")){
	exit(1);
  }
  // puts("okay"); 
  urires=parse_uri(uri,host,path,&port);

  if (!urires){
	return NULL;
  }

  //printf("parse result : %s %s %d\n", host, path, port);
  char newbuf[MAXBUF];
  char t_line[MAXLINE];

  strcpy(t_line,"");
  //start

  strcat(t_line,"GET ");                                       
  strcat(t_line, path);
  strcat(t_line," HTTP/1.0\r\n"); 

  //strcat(t_line,buf);
  Rio_readlineb(&rio,newbuf,MAXBUF); 

  while(strcmp(newbuf,"\r\n")){
 /*	
	if(!strstr(newbuf,"Connection")){
 	  strcat(t_line,newbuf);
	  Rio_readlineb(&rio,newbuf,MAXBUF);
	  continue;
 	}
 */
	strcat(t_line, newbuf);
 	Rio_readlineb(&rio,newbuf,MAXBUF);
 
  }
  strcat(t_line, "Connection: close\r\n");
  strcat(t_line, "Proxy-Connection: close\r\n");
  strcat(t_line, "\r\n");				//end


  //Aspect of client

  with_server(t_line, newbuf, host, port, fd);
  

  
  Close(fd);

  return NULL; 
}

int parse_uri(char* uri, char* host, char* path, int* port ){

  char *t_ptr= uri;
  char *tt_ptr;
  char *suffix;
  size_t t_len;

  /*
  if (!strncmp(uri,"http://",7)){
	host="\0";	//nullify the host
	return 0;
  }*/

  t_ptr +=7;
  if (!(suffix=strstr(t_ptr,"/"))){
	strcpy(path,"/");			//no suffix
  }
  else{
	strcpy(path,suffix);
  }
  
  if ((tt_ptr=strstr(t_ptr,":"))){
	if (tt_ptr[0]==':'){
	  *suffix='\0';
	  *port=atoi(tt_ptr+1);		//fill up the port
  	}
  }

  t_len= tt_ptr-t_ptr;
  strncpy(host, t_ptr, t_len);	//fill up the host
  
  return 1;
}


void with_server(char t_line[], char newbuf[], char *host, int port, int connfd){
  
  char p[MAXLINE];
  sprintf(p,"%d",port);

  int clientfd = Open_clientfd(host,p);

  //now we send it via clientfd 
  Rio_writen(clientfd, t_line, strlen(t_line));


  //catch and handle response

  rio_t newrio;
  ssize_t size; //tot_size=0;

  Rio_readinitb(&newrio,clientfd);
 
  /*
  while(strcmp(newbuf,"\r\n") && (size=Rio_readlineb(&newrio,newbuf,MAXBUF))){
	
	Rio_writen(connfd,newbuf,strlen(newbuf));		//to send the response to client
  }
  */
  while((size = Rio_readnb(&newrio,newbuf,MAXBUF)) > 0){
	Rio_writen(connfd,newbuf,size);		//to send the response to client
	
  }
  //Rio_writen(connfd,"\r\n",strlen("\r\n"));		//end

  Close(clientfd);

  return;

}


	
  

  

