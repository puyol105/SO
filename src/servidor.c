#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(){
     int fd, n_read;
     char message[1024];

     mkfifo("CV_SV", 0666);
     fd = open("CV_SV", O_RDWR);

     while((n_read = read(fd,message,1024)) > 0){ 
          write(1,message,n_read);
     }

     close(fd);

     return 0;
}
