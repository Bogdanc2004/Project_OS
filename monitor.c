#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

volatile sig_atomic_t running=1;

void handle_signal(int sig){
    FILE *fc;
    char line[100];
    switch (sig){
        case SIGUSR1:{/*here implement some code regarding hunts*/ break;}
        case SIGUSR2:{
            fc=fopen("cmd.txt", "r");
            if(fc&& fgets(line, 100, fc)!=NULL){
                line[strlen(line)-1]='\0';
                pid_t pid=fork();
                if(pid< 0){
                    perror("Fork failed");
                    exit(-1);
                }
                if(pid== 0){
                    fflush(stdout);
                    char *args[10]; // Adjust size as needed
                    int i = 0;
                    char *token = strtok(line, " ");
                    while (token != NULL && i < 9) { // Leave space for NULL terminator
                        args[i++] = token;
                        token = strtok(NULL, " ");
                    }
                    args[i] = NULL;
                    if(execvp(args[0], args)==-1){
                        perror("Exec failed");
                        exit(-1);
                    }
                    fflush(stdout);
                }
                else{
                    int status;
                    waitpid(pid, &status, 0);
                    if(WIFEXITED(status)){
                        printf("[treasure_manager] process finished: status - %d\n", WEXITSTATUS(status));
                    }
                    else{
                        printf("[treasure_manager] failed!\n");
                    }
                }

            }
            fclose(fc);
            break;}
        case SIGINT:{
            fc=fopen("cmd.txt", "r");
            if(fc&& fgets(line, 100, fc)!=NULL){
                line[strlen(line-1)]='\0';
                pid_t pid=fork();
                if(pid< 0){
                    perror("Fork failed");
                    exit(-1);
                }
                if(pid== 0){
                    char *args[]={line, NULL};
                    if(execvp(args[0], args)==-1){
                        perror("Exec failed");
                        exit(-1);
                    }
                }
                else{
                    int status;
                    waitpid(pid, &status, 0);
                    if(WIFEXITED(status)){
                        printf("[treasure_manager] process finished: status - %d\n", WEXITSTATUS(status));
                    }
                    else{
                        printf("[treasure_manager] failed!\n");
                    }
                }

            }
            fclose(fc);
            break;}
        case SIGTERM:{
            printf("Termination requested, exiting in 2s...\n");
            usleep(2000000);
            running = 0;
            break;
        }
        default:{perror("This should not happen, something went wrong during communications of signals"); exit(-1); break;}
    }
}


int main(){
    struct sigaction sa;
    sa.sa_handler=handle_signal;
    sa.sa_flags=0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    while(running){
        pause();
}
 
return 0;
}