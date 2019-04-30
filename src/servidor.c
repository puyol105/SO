#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

void update_stock(int fd, char* cv_pid, int cod, int n_stock);

int main(){
     int fdMA, fdCV, fdR, n_read, venda, codigo, precos[10], curr_index = 0, prev_stock; 
     char buff[128], buffZ[24], *token, ma_buff[128], cv_buff[128], *cv_pid;

     int fd_artigos = open("logs/ARTIGOS", O_RDONLY, 0666);
     int fd_stocks = open("logs/STOCKS", O_CREAT | O_RDWR, 0666);
     int fd_vendas = open("logs/VENDAS", O_CREAT | O_RDWR | O_APPEND, 0666);
      
     while((n_read = read(fd_artigos,buff,16)) > 0){
             token = strtok(buff," ");
             token = strtok(NULL," ");
             precos[curr_index] = atoi(token);
             curr_index++;
     }

     close(fd_artigos);

     pid_t mypid;

     switch(mypid = fork()){
          case -1:
               perror("Error Forking");
               exit(1);

          case 0:
               mkfifo("MA", 0666);
               fdMA = open("MA", O_RDWR);
               
               while((n_read = read(fdMA,ma_buff,128)) > 0){
                    token = strtok(ma_buff," ");                    

                    if((strcmp(token,"p")) == 0){
                         token = strtok(NULL," ");
                         codigo = atoi(token);
                         token = strtok(NULL," ");
                         precos[codigo] = atoi(token);
                    }

                    if((strcmp(token,"i")) == 0){
                         token = strtok(NULL," ");
                         codigo = atoi(token);
                         memset(buffZ,'\0',sizeof(buffZ));
                         sprintf(buffZ,"%d 0",codigo);
                         buffZ[23] = '\n';
                         
                         lseek(fd_stocks,codigo*24,SEEK_SET);
                         write(fd_stocks,buffZ,sizeof(buffZ));
                         lseek(fd_stocks,0,SEEK_SET);
                    }
               }

               exit(0);
                              

          default:
               mkfifo("CV", 0666);
               fdCV = open("CV", O_RDWR | O_APPEND);

               while((n_read = read(fdCV,cv_buff,128)) > 0){
                    token = strtok(cv_buff," ");
                    cv_pid = strdup(token);
                    token = strtok(NULL," ");
                    codigo = atoi(token);

                    if((token = strtok(NULL," ")) != NULL){
                         venda = atoi(token);
                         
                         lseek(fd_stocks,codigo*24,SEEK_SET);
                         read(fd_stocks,buff,24);

                         token = strtok(buff," ");
                         token = strtok(NULL," ");
                         prev_stock = atoi(token);

                         if(venda > 0){                                   
                              prev_stock += venda;
                              update_stock(fd_stocks, cv_pid, codigo, prev_stock);
                         }

                         if((venda < 0) && ((-venda) <= prev_stock)){
                              int new_stock = prev_stock + venda;
                              update_stock(fd_stocks, cv_pid, codigo, new_stock);
                         }

                         if((venda < 0) && ((-venda) > prev_stock)){
                              fdR = open(cv_pid, O_RDWR | O_APPEND);
                              int pid = atoi(cv_pid);
                              write(fdR,"Impossível efetuar venda, limitado ao stock existente\n",55);
                              kill(pid,SIGUSR1);
                              close(pid);

                         }
                    }
               }
     }
     
     close(fdMA);
     close(fdCV);
     close(fd_stocks);
     close(fd_vendas);
     
     return 0;

}

void update_stock(int fd, char* cv_pid, int cod, int n_stock){
     int pid = atoi(cv_pid);
     char stock[24];
     memset(stock,'\0',sizeof(stock));
     sprintf(stock,"%d %d",cod,n_stock);
     stock[23] = '\n';
     lseek(fd,cod*24,SEEK_SET);
     write(fd,stock,sizeof(stock));

     int fdR = open(cv_pid, O_RDWR | O_APPEND);
     write(fdR,stock,sizeof(stock));
     kill(pid,SIGUSR1);
     close(pid);
}

ssize_t readln(int fd, void* buf, size_t nbyte){
    int n = 0, r;
    char* p = (char*)buf;
    while(n<nbyte && (r=read(fd, p+n, 1))==1 && p[n] != '\n')
         n++;
    return r ==-1 ? -1 : (p != 0 && p[n] == '\n' ? n+1 : n);
}
