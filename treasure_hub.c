#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h> // Include for nanosleep
#include <errno.h>
#include <sys/stat.h> // Include for checking file existence
#include <dirent.h>

#define N 10000

pid_t monitorPid=-1;
int stopping_monitor=0;

void start_monitor (){
    if(monitorPid > 0){
        printf("Monitor is already running.\n");
        return;
    }
    int pipefd[2];
    if(pipe(pipefd) == -1){
        perror("pipe");
        exit(-1);
    }    
    monitorPid = fork();
    if (monitorPid == 0) {
        close(pipefd[0]); // Close read end in child

        // Debugging: Check if the monitor executable exists
        struct stat buffer;
        if (stat("./monitor", &buffer) == 0) {
            printf("[treasure_hub]: Found monitor executable.\n");
            fflush(stdout);
        } else {
            perror("[treasure_hub]: Monitor executable not found");
            exit(-1);
        }

        char fd_str[10];
        sprintf(fd_str, "%d", pipefd[1]);
        execl("./monitor", "monitor", fd_str, NULL);
        perror("Failed to exec monitor");
        exit(-1);
    }
    close(pipefd[1]); // Close write end in parent

    // Set the read end of the pipe to non-blocking mode
    int flags = fcntl(pipefd[0], F_GETFL, 0);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

    printf("Monitor started with PID %d\n", monitorPid);

    // Create a separate thread or process to handle non-blocking pipe reading
    if (fork() == 0) {
        char buffer[256];
        while (1) {
            ssize_t bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                printf("%s", buffer);
            } else if (bytesRead == -1 && errno != EAGAIN) {
                perror("Error reading from pipe");
                break;
            }

            // Exit the loop if the monitor process terminates
            int status;
            if (waitpid(monitorPid, &status, WNOHANG) == monitorPid) {
                printf("[treasure_hub]: Monitor terminated. Status: %d\n", WEXITSTATUS(status));
                monitorPid = -1;
                break;
            }

            struct timespec ts = {0, 100000000}; // 100ms
            nanosleep(&ts, NULL);
        }
        close(pipefd[0]);
        exit(0);
    }
    close(pipefd[0]); // Parent closes the read end
}

void stop_monitor(){
    if(monitorPid <= 0){
        printf("No monitor is currently running. \n");
        return;
    }

    kill(monitorPid, SIGTERM);
    stopping_monitor = 1;
    printf("[treasure_hub]: Sent stop signal to monitor\n");
    int status;
    struct timespec ts = {0, 100000000}; // 100ms
    while (waitpid(monitorPid, &status, WNOHANG) == 0) {
        nanosleep(&ts, NULL); // Replace usleep with nanosleep
    }
    
    printf("[treasure_hub]: Monitor terminated. Status: %d\n", WEXITSTATUS(status));
    monitorPid = -1;
    stopping_monitor = 0;
    
}

void checkMonitorStatus(){
    if(monitorPid<= 0) return;
    int status;
    pid_t result;
    if((result=waitpid(monitorPid, &status, WNOHANG))==monitorPid){
        printf("[treasure_hub]: Monitor terminated. Status: %d\n", WEXITSTATUS(status));
        monitorPid=-1;
        stopping_monitor=0;
    }
}

void writeCmd(const char *args){
    int fc=open("cmd.txt", O_WRONLY|O_CREAT|O_TRUNC, 0664);
    if(fc!=-1){
        write(fc, args, strlen(args));
        write(fc,"\n",1);
        close(fc);
    }
    else{
        perror("Cannot create command!");
        exit(-1);
    }

}

int main(int argc, char *argv[]) {
    char command[128];

    // If commands are provided as arguments, process them sequentially
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            strncpy(command, argv[i], sizeof(command) - 1);
            command[sizeof(command) - 1] = '\0';

            if (!strcmp(command, "start_monitor")) {
                start_monitor();
            } else if (!strcmp(command, "list_hunts")) {
                if (monitorPid > 0) {
                    kill(monitorPid, SIGUSR1);
                    struct timespec ts = {0, 100000000}; // 100ms
                    nanosleep(&ts, NULL);
                } else {
                    printf("[treasure_hub]: No monitor running.\n");
                }
            } else if (!strncmp(command, "list_treasures", strlen("list_treasures"))) {
                if (monitorPid > 0) {
                    char manCMD[256] = {"./treasure_manager --list"};
                    strcat(manCMD, command + strlen("list_treasures"));
                    writeCmd(manCMD);
                    kill(monitorPid, SIGUSR2);
                    struct timespec ts = {0, 100000000}; // 100ms
                    nanosleep(&ts, NULL);
                } else {
                    printf("[treasure_hub]: No monitor running.\n");
                }
            } else if (!strncmp(command, "view_treasure", strlen("view_treasure"))) {
                if (monitorPid > 0) {
                    char manCMD[256] = {"./treasure_manager --view"};
                    strcat(manCMD, command + strlen("view_treasure"));
                    writeCmd(manCMD);
                    kill(monitorPid, SIGUSR2);
                    struct timespec ts = {0, 100000000}; // 100ms
                    nanosleep(&ts, NULL);
                } else {
                    printf("[treasure_hub]: No monitor running.\n");
                }
            } else if (!strcmp(command, "stop_monitor")) {
                stop_monitor();
            } else if (!strcmp(command, "exit")) {
                if (monitorPid > 0) {
                    printf("[treasure_hub]: Error: Monitor is still running.\n");
                } else {
                    break;
                }
            } else {
                printf("[treasure_hub]: Unknown command: %s\n", command);
            }
        }
        return 0;
    }

    // Interactive mode if no arguments are provided
    while (1) {
        checkMonitorStatus();
        printf(">> ");
        fflush(stdout);
        if (!fgets(command, sizeof(command), stdin)) break;
        command[strcspn(command, "\n")] = '\0';

        if (stopping_monitor && monitorPid > 0) {
            printf("[treasure_hub]: Monitor is stopping. Please wait...\n");
            continue;
        }

        if (!strcmp(command, "start_monitor")) {
            start_monitor();
        } else if (!strcmp(command, "list_hunts")) {
            if (monitorPid > 0) {
                kill(monitorPid, SIGUSR1);
                struct timespec ts = {0, 100000000}; // 100ms
                nanosleep(&ts, NULL);
            } else {
                printf("[treasure_hub]: No monitor running.\n");
            }
        } else if (!strncmp(command, "list_treasures", strlen("list_treasures"))) {
            if (monitorPid > 0) {
                char manCMD[256] = {"./treasure_manager --list"};
                strcat(manCMD, command + strlen("list_treasures"));
                writeCmd(manCMD);
                kill(monitorPid, SIGUSR2);
                struct timespec ts = {0, 100000000}; // 100ms
                nanosleep(&ts, NULL);
            } else {
                printf("[treasure_hub]: No monitor running.\n");
            }
        } else if (!strncmp(command, "view_treasure", strlen("view_treasure"))) {
            if (monitorPid > 0) {
                char manCMD[256] = {"./treasure_manager --view"};
                strcat(manCMD, command + strlen("view_treasure"));
                writeCmd(manCMD);
                kill(monitorPid, SIGUSR2);
                struct timespec ts = {0, 100000000}; // 100ms
                nanosleep(&ts, NULL);
            } else {
                printf("[treasure_hub]: No monitor running.\n");
            }
        } else if (!strcmp(command, "stop_monitor")) {
            stop_monitor();
        } else if (!strcmp(command, "exit")) {
            if (monitorPid > 0) {
                printf("[treasure_hub]: Error: Monitor is still running.\n");
            } else {
                break;
            }
        }else if(!strcmp(command,"calculate_score")){
            if(monitorPid > 0){
                const char *huntsDir="hunts";
                DIR *dir=opendir(huntsDir);
                if(!dir){
                    perror("Directory not found!\n");
                    exit(-1);
                }
                struct dirent *entry;
                int huntCount=0;
                while((entry=readdir(dir))!=NULL){
                    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
                        continue;
                    }
                    char paths[N];
                    snprintf(paths, sizeof(paths), "./%s/%s/treasure.txt", huntsDir, entry->d_name);
                    int pipefd1[2];
                    if(pipe(pipefd1) == -1){
                        perror("pipe");
                        exit(-1);
                    }
                       
                    int scorePid=fork();
                    if(scorePid==0){
                        // Child process
                        close(pipefd1[0]); // Close read end
                        struct stat buffer;
                        if (stat("./calculate_score", &buffer) == 0) {
                            printf("[treasure_hub]: Found calculate_score executable.\n");
                            fflush(stdout);
                        } else {
                            perror("[treasure_hub]: calculate_score executable not found");
                            exit(-1);
                        }
                        printf("===%s===\n", entry->d_name);  
                        char fd1_str[10];
                        sprintf(fd1_str, "%d", pipefd1[1]);
                        execl("./calculate_score", "calculate_score", fd1_str, paths, NULL);
                        perror("Failed to exec calculate_score");
                        exit(-1);
                    } else if (scorePid > 0) {
                        // Parent process
                        close(pipefd1[1]); // Close write end
                        char scoreBuf[256];
                        ssize_t n;
                        while ((n = read(pipefd1[0], scoreBuf, sizeof(scoreBuf)-1)) > 0) {
                            scoreBuf[n] = '\0';
                            printf("%s", scoreBuf);
                        }
                        close(pipefd1[0]);
                        int status;
                        waitpid(scorePid, &status, 0);
                    } else {
                        perror("fork");
                        exit(-1);
                    }
                    
                    huntCount++;
                }
                if(huntCount==0){
                    printf("No hunts registered!");
                }

            }
            else{
               printf("[treasure_hub]: No monitor running.\n"); 
            }  
        }
        else {
            printf("[treasure_hub]: These are all the commands that you can write:\n");
            printf("\tstart_monitor\n\tlist_hunts\n\tlist_treasures [hunt]\n\tview_treasure [hunt] [treasure]\n\tcalculate_score\n\tstop_monitor\n\texit\n");
        }
    }
    return 0;
}