#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

pid_t monitorPid=-1;
int stopping_monitor=0;

void start_monitor (){
    if(monitorPid>0){
        printf("Monitor is already running.\n");
        return;
    }
    monitorPid=fork();
    if(monitorPid==0){
        execl("./monitor", "monitor", NULL);
        perror("Failed to exec monitor");
        exit(-1);
    }
    printf(" Monitor started with PID %d\n", monitorPid);
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
    while (waitpid(monitorPid, &status, WNOHANG) == 0) {
        usleep(100000); // Sleep for 100ms before checking again
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

int main(){
    char command[128];

    while(1){
        checkMonitorStatus();
        printf(">> ");
        fflush(stdout);
        if(!fgets(command, sizeof(command), stdin)) break;
        command[strcspn(command,"\n")]='\0';

        if(stopping_monitor && monitorPid>0){
            printf("[treasure_hub]: Monitor is stopping. Please wait...\n ");
            continue;
        }
        
        if(!strcmp(command, "start_monitor")){
            start_monitor();
        }
        else if(!strcmp(command, "list_hunts")){
            if(monitorPid>0){
                kill(monitorPid, SIGUSR1);
                usleep(100000);  
            }
            else
                printf("[treasure_hub]: No monitor running.\n");
        }
        else if(!strncmp(command, "list_treasures", strlen("list_treasure"))){
            if(monitorPid>0){
                char manCMD[256]={"./treasure_manager --list"};
                strcat(manCMD, command+strlen("list_treasures"));
                writeCmd(manCMD);
                kill(monitorPid, SIGUSR2);
                usleep(100000);  
            }
            else
                printf("[treasure_hub]: No monitor running.\n");
        }
        else if(!strncmp(command, "view_treasure", strlen("view_treasure"))){
            if(monitorPid>0){
                char manCMD[256]={"./treasure_manager --view"};
                strcat(manCMD, command+strlen("view_treasure"));
                writeCmd(manCMD);
                kill(monitorPid, SIGUSR2);
                usleep(100000);    
            }
            else 
                printf("[treasure_hub]: No monitor running.\n");
        }
        else if(!strcmp(command, "stop_monitor")){
            stop_monitor();
        }
        else if(!strcmp(command, "exit")){
            if(monitorPid>0){
                printf("[treasure_hub]: Error: Monitor is still running.\n");
            }
            else{
                break;
            }
        }
        else{
            printf("[treasure_hub]: These are all the commands that you can write:\n");
            printf("\tstart_monitor\n\tlist_hunts\n\tlist_treasures [hunt]\n\t\n\tview_treasure [hunt] [treasure]\n\tstop_monitor\n\texit\n");
        }

    }
    return 0;
}