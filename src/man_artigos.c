#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

//ARTIGO


//     INDICE         REF. NOME         PREÇO
//|_ _ _ _ _ _ _ _|_ _ _ _ _ _ _ _|_ _ _ _ _ _ _ _|

int main(){
     int fd_artigos, fd_strings, fdMA, sv_pid, n_read, curr_index = 0;
     char update[32], mini_buff[1024], buff[1024], command[64], artigo[16], string[128], *nome, *preco, *codigo, *token, *type;
    
     mkfifo("MA", 0666);
     fdMA = open("MA", O_WRONLY | O_APPEND);
     n_read = read(fdMA,update,10);
     update[n_read-1] = '\0';
     sv_pid = atoi(update);

     fd_artigos = open("logs/ARTIGOS", O_CREAT | O_RDWR , 0666);
     fd_strings = open("logs/STRINGS", O_CREAT | O_RDWR | O_APPEND, 0666);
     
     
     while((n_read = read(fd_artigos,buff,1024)) > 0){

          for(int i = 0; i < n_read; i++){
               if(buff[i] == '\n') curr_index++;
          }  
     }

     while((n_read = read(0,command,64)) > 0){
          command[n_read-1] = '\0';
          token = strtok(command," ");
          type = strdup(token);

          if((strcmp(type,"i")) == 0){
               
               token = strtok(NULL," ");
               nome = strdup(token);
               token = strtok(NULL," ");
               preco = strdup(token);

               lseek(fd_artigos,0,SEEK_END);
               
               memset(artigo,'\0',sizeof(artigo));
               sprintf(artigo,"%d %s",curr_index,preco);
               artigo[15] = '\n';
               write(fd_artigos,artigo,sizeof(artigo));
               
               sprintf(string,"%s\n",nome);
               write(fd_strings,string,strlen(string));
               
               sprintf(update,"i %d",curr_index);
               write(fdMA,update,strlen(update));
               
               curr_index++;
          }


          if((strcmp(type,"n")) == 0){
               char aux[32];
               token = strtok(NULL," ");
               codigo = strdup(token);
               token = strtok(NULL," ");
               nome = strdup(token);
               token = strtok(NULL," ");

               while(token){
                    sprintf(aux," %s",token);
                    strcat(nome,aux);                    
                    
                    token = strtok(NULL," ");
                    if(token == NULL) break;
               }
               
               int offset = atoi(codigo);
               
               int fd_temp = open("./logs/temp_strings",O_CREAT | O_WRONLY, 0666);
               
               lseek(fd_strings,0,SEEK_SET);

               while((n_read = read(fd_strings,mini_buff,1024)) > 0){
                   mini_buff[n_read -1] = '\0';
                   token = strtok(mini_buff,"\n");
                    
                    while(token){
                        if(offset == 0){
                              sprintf(string,"%s\n",nome);
                              write(fd_temp,string,strlen(string));   
                        }
                        else{ 
                              sprintf(string,"%s\n",token);
                              write(fd_temp,string,strlen(string));   
                        }

                        token = strtok(NULL,"\n");
                        if(token == NULL) break;

                        offset--;
                    }
               }
               
               close(fd_temp);
               close(fd_strings);
               remove("./logs/STRINGS");
               rename("./logs/temp_strings","./logs/STRINGS");
               fd_strings = open("./logs/STRINGS", O_RDWR | O_APPEND, 0666);

          }

          if((strcmp(type,"p")) == 0){
               token = strtok(NULL," ");
               codigo = strdup(token);
               token = strtok(NULL," ");
               preco = strdup(token);

               int offset = atoi(codigo);
               
               memset(artigo,'\0',sizeof(artigo));
               sprintf(artigo,"%s %s",codigo,preco);
               artigo[15] = '\n';
               lseek(fd_artigos,(offset*16),SEEK_SET);
               write(fd_artigos,artigo,sizeof(artigo));
          }
          
          if((strcmp(type,"r")) == 0){
          }
     }

     close(fd_artigos);
     close(fd_strings);
     close(fdMA);
     return 0;
}

//void insere_artigo();
//void muda_preco();
