typedef struct {
  int fd;
  int n_lines;
  int b_size;
  unsigned char *lines;
} buffer_t;


int create_buffer(struct buffer_t *buffer, int filedes, size_t size){
     buffer->fd = filedes;
     buffer->lines = malloc(size);
     if(!buffer->lines){ 
          return 0; 
     }
     buffer->b_size = size;
     buffer->n_lines = 0;
     return 1;
}

void destroy_buffer(struct buffer_t *buffer){
     if(buffer){
          if(buffer->lines) free(buffer->lines);
          free(buffer);
     }
     return;
}

ssize_t readln(struct buffer_t *buffer, void *buf, size_t nbyte){
   
}

int main(){
     int n_read = 0;
}
