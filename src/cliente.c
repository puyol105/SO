#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

ssize_t readln(int fd, char* buf);

int main(){
     int fd_sv, fd_mailbox, n_read;
     char mailbox[10], buff[128], request[256];
     fd_sv = open("SV", O_WRONLY);
     int p = getpid();

     sprintf(mailbox,"%d",getpid());
     mkfifo(mailbox, 0666);

     pid_t pid;

     if((pid = fork()) == 0){
          fd_mailbox = open(mailbox,O_RDONLY);

          while((n_read = read(fd_mailbox,buff,256)) >= 0){
               write(1,buff,n_read);
          }     

          close(fd_mailbox);  
          _exit(0);
     }
               
     while((n_read = readln(0,buff)) > 0){
          buff[n_read - 1] = '\0';
          sprintf(request,"CV %d %s\n",p,buff);
          write(fd_sv,request,strlen(request));
     }   
     
     kill(pid,SIGKILL);
     close(fd_sv);
     execlp("rm","rm",mailbox,NULL);
}

ssize_t readln(int fildes, char* buf){
    ssize_t total_char = 0, r;

    while((r = read(fildes, buf + total_char, 1)) > 0){
        if(buf[total_char++] == '\n') break;
    }

    return total_char;
}