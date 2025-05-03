#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#define READ_BUF_SIZE 128
#define MAX_PATH 600
#define MAX_LINE 256

volatile sig_atomic_t running=1;

char *my_fgets(char *buf, int maxlen, int fd) {
    static char internal_buf[READ_BUF_SIZE];
    static int buf_pos = 0, buf_len = 0;

    int i = 0;

    while (i < maxlen - 1) {
        if (buf_pos >= buf_len) {
            // refill internal buffer
            buf_len = read(fd, internal_buf, READ_BUF_SIZE);
            if (buf_len <= 0) {
                if (i == 0) return NULL; // EOF or error before any char
                break; // return what we read so far
            }
            buf_pos = 0;
        }

        char c = internal_buf[buf_pos++];
        buf[i++] = c;
        if (c == '\n') break;
    }

    buf[i] = '\0'; // null-terminate
    return buf;
}

void handle_signal(int sig){
    int fc;
    char line[100];
    switch (sig){
        case SIGUSR1:{
            const char *huntsDir="hunts";
            DIR *dir=opendir(huntsDir);
            if(!dir){
                perror("Failed to open hunts directory");
                exit(-1);
            }
            struct dirent *entry;
            int huntCount = 0;
            while ((entry = readdir(dir)) != NULL) {
                if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
                    continue;
                }

                char dirPath[MAX_PATH];
                snprintf(dirPath, sizeof(dirPath), "%s/%s", huntsDir, entry->d_name);

                struct stat statbuf;
                if (stat(dirPath, &statbuf) == -1) {
                    perror("Failed to stat directory entry");
                    continue;
                }

                if (S_ISDIR(statbuf.st_mode)) {
                    char filePath[MAX_PATH + 20]; // Increase buffer size to accommodate "/treasure.txt" safely
                    snprintf(filePath, sizeof(filePath), "%s/treasure.txt", dirPath);
                    if (strlen(filePath) >= sizeof(filePath)) {
                        fprintf(stderr, "Error: File path truncated: %s\n", filePath);
                        continue;
                    }
                    int fd = open(filePath, O_RDONLY);
                    if (fd == -1) {
                        // Skip if treasure.txt does not exist
                        continue;
                    }
                    int treasureCount = 0;
                    char line[MAX_LINE];
                    while (my_fgets(line, sizeof(line), fd) != NULL) {
                        treasureCount++;
                    }
                    close(fd);
                    printf("%s --- %d treasures\n", entry->d_name, treasureCount);
                    huntCount++;
                }
            }
            closedir(dir);
            if(huntCount==0){
                fprintf(stderr, "Error: No hunts found.\n");
            }
            
            break;}
        case SIGUSR2:{
            fc=open("cmd.txt", O_RDONLY);
            if(fc!=-1 && my_fgets(line, 100, fc)!=NULL){
                line[strlen(line)-1]='\0';
                pid_t pid=fork();
                if(pid< 0){
                    perror("Fork failed");
                    exit(-1);
                }
                if(pid== 0){
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
            close(fc);
            break;}
        case SIGINT:{
            fc=open("cmd.txt", O_RDONLY);
            if(fc!=-1&& my_fgets(line, 100, fc)!=NULL){
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
            close(fc);
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
        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGUSR1);
        sigaddset(&sigset, SIGUSR2);
        sigaddset(&sigset, SIGINT);
        sigaddset(&sigset, SIGTERM);

        int sig;
        sigwait(&sigset, &sig); // Wait for a signal
        handle_signal(sig);     // Handle the signal
    }
 
    return 0;
}