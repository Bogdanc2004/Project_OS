#define _XOPEN_SOURCE 700 // Ensure compatibility for POSIX features
#define _POSIX_C_SOURCE 200809L // Explicitly define POSIX version for usleep

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // Ensure this is included for usleep
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h> // Include for nanosleep

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

volatile int out_fd = STDOUT_FILENO; // Use pipe file descriptor for output

void handle_signal(int sig) {
    int fc;
    char line[100];
    switch (sig) {
        case SIGUSR1: {
            const char *huntsDir = "hunts";
            DIR *dir = opendir(huntsDir);
            if (!dir) {
                dprintf(out_fd, "Error: Failed to open hunts directory.\n");
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
                    char filePath[MAX_PATH + 20];
                    snprintf(filePath, sizeof(filePath), "%s/treasure.txt", dirPath);
                    if (strlen(filePath) >= sizeof(filePath)) {
                        dprintf(out_fd, "Error: File path truncated: %s\n", filePath);
                        continue;
                    }
                    int fd = open(filePath, O_RDONLY);
                    if (fd == -1) {
                        dprintf(out_fd, "Warning: No treasures found in %s.\n", entry->d_name);
                        continue;
                    }
                    int treasureCount = 0;
                    char line[MAX_LINE];
                    while (my_fgets(line, sizeof(line), fd) != NULL) {
                        treasureCount++;
                    }
                    close(fd);
                    dprintf(out_fd, "%s --- %d treasures\n", entry->d_name, treasureCount);
                    huntCount++;
                }
            }
            closedir(dir);
            if (huntCount == 0) {
                dprintf(out_fd, "Error: No hunts found.\n");
            }
            break;
        }
        case SIGUSR2: {
            fc = open("cmd.txt", O_RDONLY);
            if (fc != -1 && my_fgets(line, 100, fc) != NULL) {
                line[strlen(line) - 1] = '\0';
                pid_t pid = fork();
                if (pid < 0) {
                    perror("Fork failed");
                    close(fc);
                    return;
                }
                if (pid == 0) {
                    char *args[10]; // Adjust size as needed
                    int i = 0;
                    char *token = strtok(line, " ");
                    while (token != NULL && i < 9) { // Leave space for NULL terminator
                        args[i++] = token;
                        token = strtok(NULL, " ");
                    }
                    args[i] = NULL;
                    if (execvp(args[0], args) == -1) {
                        perror("Exec failed");
                        exit(-1);
                    }
                } else {
                    int status;
                    waitpid(pid, &status, 0);
                    if (WIFEXITED(status)) {
                        dprintf(out_fd, "[treasure_manager] process finished: status - %d\n", WEXITSTATUS(status));
                    } else {
                        dprintf(out_fd, "[treasure_manager] process failed!\n");
                    }
                }
            } else {
                dprintf(out_fd, "Error: Failed to read cmd.txt.\n");
            }
            close(fc);
            unlink("cmd.txt");
            break;
        }
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
                        dprintf(out_fd,"[treasure_manager] process finished: status - %d\n", WEXITSTATUS(status));
                    }
                    else{
                        dprintf(out_fd,"[treasure_manager] failed!\n");
                    }
                }

            }
            close(fc);
            unlink("cmd.txt");
            break;}
        case SIGTERM: {
            dprintf(out_fd, "Termination requested, exiting in 2s...\n");
            struct timespec ts = {2, 0}; // 2 seconds, 0 nanoseconds
            nanosleep(&ts, NULL);
            running = 0;
            break;
        }
        default: {
            dprintf(out_fd, "Error: Unexpected signal received: %d\n", sig);
            break;
        }
    }
}


int main(int argc, char **argv){
    if (argc > 1) {
        out_fd = atoi(argv[1]); // Read pipe file descriptor
        if (out_fd <= 0) {
            fprintf(stderr, "Error: Invalid pipe file descriptor.\n");
            return -1;
        }
    }

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    while (running) {
        pause(); // Wait for signals
    }

    return 0;
}