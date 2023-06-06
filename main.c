#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#define PERMS 0666
#define DEBUGGING 0
#define TRUE 1

int token = -1;
int parent;
int *ids;
int ** pipetas;
volatile char lock =0;

sem_t sem;

void trancar(){
    label:{
        char local = 1;
        char tmp = lock;
        lock = local;
        local =tmp;
    
        if(0 != local){
            goto label;
        }
    }
}

void destrancar(){
    lock = 0;
}

int main (int argc,char * argv[] )
{
      
    //signal(SIGINT, stopHandler);
    parent = getpid();
    int numDePipetas= atoi(argv[1]), max = atoi(argv[4]), tempoDeEspera= atoi(argv[3]);
    
    float prob= atof(argv[2]);
    int probInt = (int) (100 * prob); 
    int pid   = -1; 
    ids= malloc(numDePipetas * sizeof(int));
    char nomes[numDePipetas][15];
    int len = sizeof(int *) * numDePipetas + sizeof(int) * 2 * numDePipetas;
    pipetas = (int **) malloc(len);
    int * ptr = ( int *) (pipetas+numDePipetas);

    for(int i = 0; i< numDePipetas; i++){
        pipetas[i] = (ptr + 2 * i);
    }
    
    for(int i = 0; i< numDePipetas; i++){
        char * buffer = (char *) malloc(15);

        if(i!=numDePipetas-1){
            sprintf(buffer,"pipe%dto%d",i+1,i+2);
        }else  sprintf(buffer,"pipe%dto%d",numDePipetas,1);

        mkfifo(buffer,PERMS);
        strncpy(nomes[i],buffer,15);
        memset(buffer,0,15);
        free(buffer);

        if(DEBUGGING){ printf("CRIAR PIPE\n");}
    }
    
    for(int i = 0; i< numDePipetas  ;i ++){
        pid = fork();
        if(pid == 0){ // processo filho
            if(DEBUGGING){ printf("CRIAR FILHO\n");}
            ids[i] = getpid();
            unsigned int seed = ids[i];
            srand(seed);
            if(DEBUGGING){ printf("STACK?\n");}
            //abrir pipes
            if(i==numDePipetas-1){
                pipetas[i][0] = open(nomes[i], O_RDONLY);
                pipetas[i][1] = open(nomes[0],O_WRONLY);
            }
            else{
                pipetas[i][0] = open(nomes[i], O_RDONLY);
                pipetas[i][1] = open(nomes[i+1],O_WRONLY);
            }
            
            
        
            if(DEBUGGING){ printf("STACK?\n");}
        
            while(1){
                read(pipetas[i][0], &token, sizeof(int));
                // printf("[p%d] received token (val = %d)\n", i + 1, token);
                if(token >= max){
                    for(int k=0;k<numDePipetas;k++){
                        close(pipetas[k][1]);
                        close(pipetas[k][0]);
                        unlink(nomes[k]);
                    }
                
                    for(int k=0;k<numDePipetas;k++ ){
                        if(k!=i){kill(ids[k],SIGINT);}
                    } 
                    fflush(stdout);
                    return 0;         
                }
             


                if((rand() % 100 + 1 ) < probInt){
                    printf("[p%d] blocked on token (val = %d) PROCESSO = %d\n",i+1,token, getpid());
                    sleep(tempoDeEspera);
                    fflush(stdout);
                }

                if(token >= max){
                    for(int k=0;k<numDePipetas;k++){
                        close(pipetas[k][1]);
                        close(pipetas[k][0]);
                        unlink(nomes[k]);
                    }
                
                    for(int k=0;k<numDePipetas;k++ ){
                        if(k!=i){kill(ids[k],SIGINT);}
                    } 
                    fflush(stdout);
                    return 0;         
                }
             
                int novoToken = token + 1;
                write(pipetas[i][1], &novoToken, sizeof(int)); 
            }

        }
      
    }

    int fd_write = open("pipe1to2", O_WRONLY);
    token = 0;
    write(fd_write, &token, sizeof(int));
    
    for(int i = 0; i < numDePipetas; i++){
        wait(NULL);
    }
    
    return !TRUE;
}