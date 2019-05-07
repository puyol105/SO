#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

void insere_artigo(int fd_a, int fd_s, int index, int ref, char* nome, double preco);
void change_price(int fd_a, int index, double preco);
void change_name(int fd_a, int fd_s, int index, int ref, char* nome);
bool check_artigo(int fd_s, char* nome);
bool is_number(const char *str);
ssize_t readln(int fd, void* buf, size_t nbyte);

//ARTIGO

//     INDICE         REF. NOME         PREÇO
//|_ _ _ _ _ _ _ _|_ _ _ _ _ _ _ _|_ _ _ _ _ _ _ _|

int main(){
     int fd_artigos, fd_strings, fd_sv, n_read, curr_index = 0, curr_ref = 0, codigo;
     double preco;
     char buff[1024], command[64], artigo[32], string[128], message[32];
     char *token, *nome;

     fd_artigos = open("logs/ARTIGOS", O_CREAT | O_RDWR , 0666);
     fd_strings = open("logs/STRINGS", O_CREAT | O_RDWR | O_APPEND, 0666);
     fd_sv = open("SV", O_WRONLY | O_APPEND , 0666);
          
     while((n_read = readln(fd_artigos,buff,32)) > 0){
          curr_index++;
     }

     while((n_read = readln(fd_strings,buff,128)) > 0){
          curr_ref++;
     }


     while((n_read = readln(0,command,64)) > 0){
          command[n_read-1] = '\0';
          //preco = strdup(strrchr(command,' ')+1);
          token = strtok(command," ");

          if(strcmp(token,"i") == 0){
                    char *ptr;
                    token = strtok(NULL," ");
                    nome = strdup(token);
                    token = strtok(NULL," ");

                    if((strlen(token) <= 16) && (is_number(token))){
                         preco = strtod(token,&ptr);
                         
                         if(check_artigo(fd_strings,nome)){
                              insere_artigo(fd_artigos,fd_strings,curr_index,curr_ref,nome,preco);
                              strcpy(message,"MA i \n");
                              write(fd_sv,message,strlen(message));
                         
                              curr_index++; curr_ref++;
                         }else{
                              write(1,"Artigo já existe!\n",19);
                         }
                    
                    }else{
                         write(1,"Preço Inválido\n",18);
                    }
                     
          }else if(strcmp(token,"n") == 0){
                    token = strtok(NULL," ");
                    codigo = atoi(token);
                    token = strtok(NULL," ");
                    nome = strdup(token);
                 
                    curr_ref++;
                    if(codigo <= curr_index) change_name(fd_artigos,fd_strings,codigo,curr_ref,nome);
                    else write(1,"Código Inválido\n",18);

          }else if(strcmp(token,"p") == 0){
                    char *ptr;             
                    token = strtok(NULL," ");
                    codigo = atoi(token);
                    token = strtok(NULL," ");

                    if((strlen(token) <= 16) && (is_number(token))){
                         preco = strtod(token,&ptr);
                    }else{
                         write(1,"Preço Inválido\n",18);
                    }
               
                    change_price(fd_artigos,codigo,preco);

                    sprintf(message,"MA p %d %.2f\n",codigo,preco);
                    write(fd_sv,message,strlen(message));
     
          }else if(strcmp(token,"a") == 0){
                    sprintf(message,"MA a\n");
                    write(fd_sv,message,strlen(message));
          }else{
               write(1,"Comando Inválido\n",19);
          }
     }

     close(fd_artigos);
     close(fd_strings);
     close(fd_sv);
     return 0;

}

void insere_artigo(int fd_a,int fd_s,int index, int ref, char *nome, double preco){
     char artigo[32];
     char string[128];
     
     memset(artigo,'\0',sizeof(artigo));
     sprintf(artigo,"%d %d %.2f",index,ref,preco);
     artigo[31] = '\n';

     lseek(fd_a,0,SEEK_END);
     write(fd_a,artigo,sizeof(artigo));

     sprintf(string,"%s\n",nome);
     write(fd_s,string,strlen(string));
}

void change_price(int fd_a, int index, double preco){
     char artigo[32];
     char *token, *ref;
     int offset = index * 32;

     lseek(fd_a,offset,SEEK_SET);
     read(fd_a,artigo,32);
     token = strtok(artigo," ");
     token = strtok(NULL," ");
     ref = strdup(token); 

     memset(artigo,'\0',sizeof(artigo));
     sprintf(artigo,"%d %s %.2f",index,ref,preco);
     artigo[31] = '\n';

     lseek(fd_a,offset,SEEK_SET);
     write(fd_a,artigo,sizeof(artigo));
}

void change_name(int fd_a, int fd_s, int index, int ref, char *nome){
     char artigo[32], string[128];
     char *token, *preco;
     int offset = index * 32;

     sprintf(string,"%s\n",nome);
     write(fd_s,string,strlen(string));

     lseek(fd_a,offset,SEEK_SET);
     read(fd_a,artigo,32);
     token = strtok(artigo," ");
     token = strtok(NULL," ");
     token = strtok(NULL," ");
     preco = strdup(token); 

     memset(artigo,'\0',sizeof(artigo));
     sprintf(artigo,"%d %d %s",index,ref,preco);
     artigo[31] = '\n';

     lseek(fd_a,offset,SEEK_SET);
     write(fd_a,artigo,sizeof(artigo));
}

bool check_artigo(int fd_s, char* nome){
     char buff[512];
     int n_read = 0;
     
     lseek(fd_s,0,SEEK_SET);
     while((n_read = readln(fd_s,buff,512)) > 0){
          buff[n_read -1] = '\0';
          if(strcmp(buff,nome) == 0) { return false; }
     }

     return true;
}

bool is_number(const char *str){
     int decimal_separator = 0;

     if(str == NULL || *str == '\0'){ return false; }
     
     while(*str){
          char c = *str;

          switch(c){
          
          case '.':
               decimal_separator++;
               if(decimal_separator > 1) return false;
               break;
          
          default:
               if(c < '0' || c > '9') { return false; }
          
          }
          str++;
     }    

     return true;
}

ssize_t readln(int fd, void* buf, size_t nbyte){
    int n = 0, r;
    char* p = (char*)buf;
    while(n<nbyte && (r=read(fd, p+n, 1))==1 && p[n] != '\n')
         n++;
    return r ==-1 ? -1 : (p != 0 && p[n] == '\n' ? n+1 : n);
}