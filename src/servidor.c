#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#define FATHER_READ pipe_IN[0]
#define FATHER_WRITE pipe_OUT[1]
#define SON_READ pipe_OUT[0]
#define SON_WRITE pipe_IN[1]

void update_stock(int fd, int cod, int n_stock);
void update_vendas(int fd_a, int fd_v, char* cv_mailbox, int cod, int qtd);
ssize_t readln(int fd, void* buf, size_t nbyte);

int main(){
     int fd_mailbox, n_read, curr_index = -1, curr_vendas = 0, curr_agreg = 0; 
     char buff[128], message[256], *token;

     int fd_artigos = open("logs/ARTIGOS", O_CREAT | O_RDONLY, 0666);
     int fd_stocks = open("logs/STOCKS", O_CREAT | O_RDWR, 0666);
     int fd_vendas = open("logs/VENDAS", O_CREAT | O_RDWR | O_APPEND, 0666);
      
     while((n_read = read(fd_artigos,buff,32)) > 0){
          curr_index++;
     }

     while((n_read = read(fd_vendas,buff,32)) > 0){
          curr_vendas++;
     }

     mkfifo("SV", 0666);
     int fd_sv = open("SV", O_RDONLY);

     while((n_read = readln(fd_sv,buff,128)) >= 0){
          buff[n_read -1] = '\0';
          token = strtok(buff," ");
     
          // Tratamento de mensagens provenientes da "Manutenção de Artigos"
          if(strcmp(token,"MA") == 0){
               int codigo;
               double preco;
               char *ptr;
               pid_t son;
               int pipe_IN[2], pipe_OUT[2];
               char agreg[32];
               char index[10];
          
               token = strtok(NULL," ");
               switch(token[0]){
                    case 'i':
                         update_stock(fd_stocks, curr_index, 0);
                         curr_index++;
                         break;

                    case 'p':
                         //alterar precos na cache, se existir
                         token = strtok(NULL," ");
                         codigo = atoi(token);
                         token = strtok(NULL," ");
                         preco = strtod(token,&ptr);
                         break;

                    case 'a':

                         if(curr_vendas == 0){
                              sprintf(message,"Sem vendas para agregar!\n"); 
                              write(1,message,strlen(message));
                              break;
                         }

                         pipe(pipe_IN);
                         pipe(pipe_OUT);

                         if((son = fork()) == 0){
                              // Fazer os pipes parte do stdin e do stdout
                              dup2(SON_READ,0);
                              dup2(SON_WRITE,1);
                              close(SON_READ);
                              close(SON_WRITE);
                              close(FATHER_READ);
                              close(FATHER_WRITE);

                              char index[10];
                              sprintf(index,"%d",curr_index);

                              // Executar o agregador 
                              char* param[] = {"./ag"};
                              execlp("./ag","./ag",index,(char *) NULL);
                              exit(1);
                         }

                         close(SON_READ);
                         close(SON_WRITE);

                         lseek(fd_vendas,curr_agreg*32,SEEK_SET);
                         while(read(fd_vendas,agreg,32) > 0){
                              write(FATHER_WRITE,agreg,32);
                              curr_agreg++;
                         }

                         close(FATHER_WRITE);

                         time_t t = time(NULL);
                         struct tm *timeinfo = localtime(&t);
                         char file_agreg[64];
                         strftime(file_agreg,64,"./%Y-%m-%dT%X",timeinfo);
                         int fd_agreg = open(file_agreg,O_CREAT | O_WRONLY | O_APPEND,0666);
                         
                         while((n_read = read(FATHER_READ,agreg,32)) > 0){
                              write(fd_agreg,agreg,32);
                         }   

                         close(FATHER_READ);
                         close(fd_agreg);
                         break;

                    //case 'r':
                    //   curr_index--;
                    //   verificar se este estava na cache
                    //   break;

                    default: 
                         write(1,"Erro na mensagem entre MA e SV\n\n",32);
                         break;
               }          
          }
     
          // Tratamento de mensagens provenientes de um "Cliente de Vendas"
          if(strcmp(token,"CV") == 0){
               char *cv_mailbox, *stock, *tk_aux, string[128];
               int codigo, cv_pid;
               
               token = strtok(NULL," ");
               cv_mailbox = strdup(token);
               cv_pid = atoi(cv_mailbox);
               token = strtok(NULL," ");
               codigo = atoi(token);

               // Obtemos o Stock atual do Artigo
               lseek(fd_stocks,codigo*24,SEEK_SET);
               read(fd_stocks,string,24);
               tk_aux = strrchr(string,' ') + 1;
               stock = strdup(tk_aux);
               token = strtok(NULL," ");

               if(codigo > curr_index) {sprintf(message,"Artigo Inexistente!\n\n");}
               else{
                    if(token){
                         int venda = atoi(token);
                         int n_stock = atoi(stock);
                         if(venda > 0){                                   
                              // Entrada de Stock
                              n_stock += venda;
                              update_stock(fd_stocks, codigo, n_stock);
                              sprintf(message,"Entrada em Stock efetuada com sucesso!\n\n");
                         }
                         if((venda < 0) && ((-venda) <= n_stock)){
                              // Venda de Artigo
                              update_stock(fd_stocks, codigo, n_stock + venda);
                              update_vendas(fd_artigos, fd_vendas, cv_mailbox, codigo, -venda);   
                              sprintf(message,"Venda efetuada com sucesso!\n\n");
                         }
                         if((venda < 0) && ((-venda) > n_stock)){
                              // Impossibilidade de venda
                              sprintf(message,"Impossível efetuar venda, limitado ao stock existente!\n\n");
                         }
                    }else{
                         // Pedido de informação de Artigo (Código, Preço, e Stock)
                         char artigo[32], *preco;
                         lseek(fd_artigos,codigo*32,SEEK_SET);
                         read(fd_artigos,artigo,32);
                         preco = strrchr(artigo,' ') + 1;
                         sprintf(message,"Artigo: %d\nPreço: %s\nStock: %s\n\n",codigo,preco,stock);
                    }

               }
               
               fd_mailbox = open(cv_mailbox, O_WRONLY);
               write(fd_mailbox,message,strlen(message));
               close(fd_mailbox);
          }    
     }

     close(fd_artigos);
     close(fd_stocks);
     close(fd_vendas);
     close(fd_sv);
     
     return 0;
}

void update_stock(int fd, int cod, int n_stock){
     char stock[24];
     memset(stock,'\0',sizeof(stock));
     sprintf(stock,"%d %d",cod,n_stock);
     stock[23] = '\n';
     
     lseek(fd,cod*24,SEEK_SET);
     write(fd,stock,sizeof(stock));
}

void update_vendas(int fd_a, int fd_v, char* cv_mailbox, int cod, int qtd){
     int cv_pid = atoi(cv_mailbox);
     double montante;
     char venda[32], artigo[32], *token, *ptr;
     
     lseek(fd_a,cod*32,SEEK_SET);
     read(fd_a,artigo,32);
     token = strrchr(artigo,' ') + 1;
     montante = strtod(token,&ptr) * qtd;

     memset(venda,'\0',sizeof(venda));
     sprintf(venda,"%d %d %.2f",cod,qtd,montante);
     venda[31] = '\n';
     write(fd_v,venda,sizeof(venda));
}

ssize_t readln(int fd, void* buf, size_t nbyte){
    int n = 0, r;
    char* p = (char*)buf;
    while(n<nbyte && (r=read(fd, p+n, 1))==1 && p[n] != '\n')
         n++;
    return r ==-1 ? -1 : (p != 0 && p[n] == '\n' ? n+1 : n);
}