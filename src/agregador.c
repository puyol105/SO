#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

ssize_t readln(int fd, void* buf, size_t nbyte);

int main(int argc, char**argv){
    int n_read = 0;
    int codigo, quantidade, qtd_aux;
    float montante, mtt_aux;
    char venda[32], *aux, *token, *ptr;
    
    int size = atoi(argv[1]);
    
    char agregacoes[size][32];

    for(int i = 0; i < size; i++){
        memset(agregacoes[i],'\0',32);
        sprintf(agregacoes[i],"NULL");
    }


    while(n_read = read(0,venda,32) > 0){
        aux = strdup(venda);

        token = strtok(aux," ");
        codigo = atoi(token);
        token = strtok(NULL," ");
        quantidade = atoi(token);
        token = strtok(NULL," ");
        montante = strtof(token,&ptr);

        if(strcmp(agregacoes[codigo],"NULL") == 0){
            sprintf(agregacoes[codigo],"%s",venda);
        }
        else{
            aux = strdup(agregacoes[codigo]);

            token = strtok(aux," ");
            token = strtok(NULL," ");
            qtd_aux = atoi(token) + quantidade;
            token = strtok(NULL," ");
            mtt_aux = strtof(token,&ptr) + montante;

            sprintf(agregacoes[codigo],"%d %d %.2f",codigo,qtd_aux,mtt_aux);
        }
    }

    for(int j = 0; j<size; j++){
        if(!strcmp(agregacoes[j],"NULL") == 0){
            agregacoes[j][31] = '\n';
            write(1,agregacoes[j],32);
        }
    }
    
    close(1);

    return 0;
}

ssize_t readln(int fd, void* buf, size_t nbyte){
    int n = 0, r;
    char* p = (char*)buf;
    while(n<nbyte && (r=read(fd, p+n, 1))==1 && p[n] != '\n')
         n++;
    return r ==-1 ? -1 : (p != 0 && p[n] == '\n' ? n+1 : n);
}