#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

void response_handler(int signum){
     char r_pid[10];
     char response[100];
     sprintf(r_pid,"%d",getpid());

     int fd_pid = open(r_pid, O_RDWR, 0666);
     int r = read(fd_pid,response,100);
     write(1,response,r);
     close(fd_pid);
}

int main(){
     int fdCV, pret, n_read, mypid;
     char request[256];
     char mailbox[10], buff[128];
     int r;

     mkfifo("CV", 0666);
     fdCV = open("CV", O_RDWR | O_APPEND);
     mypid = getpid();
     sprintf(mailbox,"%d",mypid);
     mkfifo(mailbox, 0666);

     signal(SIGUSR1,response_handler);
     while((n_read = read(0,buff,128)) > 0){
          sprintf(request,"%d %s",mypid,buff);
          write(fdCV,request,strlen(request));
     }
     
     close(fdCV);
     return 0;
}
