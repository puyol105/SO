#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

ssize_t readln(int fd, void* buf, size_t nbyte);

int main(){
     int fd_sv, fd_mailbox, n_read;
     char mailbox[10], buff[128], request[256];
     fd_sv = open("SV", O_WRONLY);

     sprintf(mailbox,"%d",getpid());
     mkfifo(mailbox, 0666);

     pid_t pid;

     if((pid = fork()) == 0){
          fd_mailbox = open(mailbox,O_RDONLY);

          while((n_read = read(fd_mailbox,buff,256)) >= 0){
               write(1,buff,n_read);
          }     

          _exit(0);
     }
               
     while((n_read = readln(0,buff,128)) > 0){
          buff[n_read - 1] = '\0';
          sprintf(request,"CV %d %s\n",getpid(),buff);
          write(fd_sv,request,strlen(request));
     }   
     
     kill(pid,SIGKILL);
     execlp("rm","rm",mailbox,NULL);
}

ssize_t readln(int fd, void* buf, size_t nbyte){
    int n = 0, r;
    char* p = (char*)buf;
    while(n<nbyte && (r=read(fd, p+n, 1))==1 && p[n] != '\n')
         n++;
    return r ==-1 ? -1 : (p != 0 && p[n] == '\n' ? n+1 : n);
}