#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
int agregacao(int curr_index, int curr_vendas, int curr_agreg, int fd_vendas);
void agregacao_by_N(int curr_index, int curr_vendas, int curr_agreg, int fd_vendas, int n_agregadores);
bool is_number(const char *str);
ssize_t readln(int fd, char* buf);

int main(){
     int fd_mailbox, n_read, curr_index = 0, curr_vendas = 0, curr_agreg = 0; 
     char buff[128], message[256], *token, *aux;

     struct stat st = {0};

     if (stat("./logs", &st) == -1) {
          mkdir("./logs", 0700);
     }

     if (stat("./aggregation", &st) == -1) {
          mkdir("./aggregation", 0700);
     }

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

     while((n_read = readln(fd_sv,buff)) >= 0){
          int space = 0;
          buff[n_read -1] = '\0';
          
          for(int j = 0;j<strlen(buff);j++){
               if (buff[j] == ' ') space++;
          }
     
          token = strtok(buff," ");
     
          // Tratamento de mensagens provenientes da "Manutenção de Artigos"
          if(strcmp(token,"MA") == 0){
               int codigo, n;
               double preco;
               char *ptr;

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

                         if(space > 1){
                              token = strtok(NULL," ");
                              n = atoi(token);
                              agregacao_by_N(curr_index,curr_vendas,curr_agreg,fd_vendas,n);
                         }else{
                              curr_agreg = agregacao(curr_index,curr_vendas,curr_agreg,fd_vendas);
                         }
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

ssize_t readln(int fildes, char* buf){
    ssize_t total_char = 0, r;

    while((r = read(fildes, buf + total_char, 1)) > 0){
        if(buf[total_char++] == '\n') break;
    }

    return total_char;
}

void agregacao_by_N(int curr_index, int curr_vendas, int curr_agreg, int fd_vendas, int n_agregadores){
     pid_t son;
     int n_read = 0, i;
     char agreg[32], index[10], message[64];

     int pipe_OUT[n_agregadores][2];
     int pipe_IN[n_agregadores][2];

     int n_agregacoes = (curr_vendas - curr_agreg) / n_agregadores;

     if(curr_vendas == 0){
          sprintf(message,"Sem vendas para agregar!\n"); 
          write(1,message,strlen(message));
          exit(0);
     }

     for(i = 0;i < n_agregadores;i++){

          pipe(pipe_OUT[i]);
          pipe(pipe_IN[i]);

          if((son = fork()) == 0){
               // Redirecionamento de pipes para stdin e stdout
               dup2(pipe_OUT[i][0],0);
               dup2(pipe_IN[i][1],1);
               close(pipe_OUT[i][0]);
               close(pipe_IN[i][1]);
               close(pipe_IN[i][0]);
               close(pipe_OUT[i][1]);

               sprintf(index,"%d",curr_index);
               // Executar o agregador 
               execlp("./ag","./ag",index,(char *) NULL);
               exit(1);
          }
     }
     
     lseek(fd_vendas,curr_agreg*32,SEEK_SET);
     
     for(i = 0;i < n_agregadores;i++){

          if(i == n_agregadores-1){
               while((n_read = read(fd_vendas,agreg,32)) > 0){
                    write(pipe_OUT[i][1],agreg,32);
                    curr_agreg++;
               }
          }else{
               for(int j=0;j < n_agregacoes;j++){
                    read(fd_vendas,agreg,32);
                    write(pipe_OUT[i][1],agreg,32);
                    curr_agreg++;
               }
          }

          close(pipe_OUT[i][0]);
          close(pipe_IN[i][1]);
          close(pipe_OUT[i][1]);    
     }

     int fd_temp = open("./aggregation/temp", O_CREAT | O_RDWR | O_TRUNC, 0666);

     for(i = 0;i<n_agregadores;i++){
          
          while((n_read = read(pipe_IN[i][0],agreg,32)) > 0){
               write(fd_temp,agreg,32);
          }
          
          close(pipe_IN[i][0]);
     }

     close(fd_temp);
}

int agregacao(int curr_index, int curr_vendas, int curr_agreg, int fd_vendas){
     pid_t son;
     int pipe_IN[2], pipe_OUT[2], n_read = 0;
     char agreg[32], index[10], message[64];
     
     if(curr_vendas == 0){
          sprintf(message,"Sem vendas para agregar!\n"); 
          write(1,message,strlen(message));
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
          
          sprintf(index,"%d",curr_index);
          // Executar o agregador 
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
     char file_agreg[128];
     strftime(file_agreg,128,"./aggregation/%Y-%m-%dT%X",timeinfo);
     
     int fd_agreg = open(file_agreg, O_CREAT | O_WRONLY | O_APPEND,0666);
     
     while((n_read = read(FATHER_READ,agreg,32)) > 0){
          write(fd_agreg,agreg,32);
     }   

     close(FATHER_READ);
     close(fd_agreg);

     return curr_agreg;
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