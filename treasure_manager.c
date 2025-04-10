#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#define N 10000
#define D 200
#define READ_BUF_SIZE 128

typedef struct {
    char treasureID[D];
    char userName[D];
    float latitude;
    float longitude;
    char clue[D];
    int value;

}Treasure;

void writeInTreasureFile(int fd, int fl,char *treasure){
    Treasure t;
    char line[N]="";
    char line1[256]="";
    printf("Now, let's add the treasure: %s\n", treasure);
    //printf("Treasure id: "); scanf("%s", t.treasureID);fflush(stdout);  printf("\n");
    strcpy(t.treasureID, treasure);
    printf("User: "); scanf("%s", t.userName); fflush(stdout);  printf("\n");
    printf("Latitude: "); scanf("%f", &t.latitude);fflush(stdout);   printf("\n");
    printf("Longitude: "); scanf("%f", &t.longitude); fflush(stdout); printf("\n");
    printf("Clue: "); scanf("%s", t.clue); fflush(stdout); printf("\n");
    printf("Value:" ); scanf("%d", &t.value); fflush(stdout); printf("\n");
    sprintf(line, "%s, %s, %f, %f, %s, %d\n",t.treasureID, t.userName, t.latitude, t.longitude, t.clue, t.value);
    if(write(fd, line, strlen(line))==-1)
        perror("Cannot write");
    sprintf(line1, "%s was recorded by user %s.\n", treasure, t.userName);
    if(write(fl, line1, strlen(line1))==-1)
        perror("Cannot record in log-file");
}

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

int theTreasureExists(char* treasure) {
    char line[N];
    int fd = open("treasure.txt", O_RDONLY);
    if (fd == -1) {
        perror("Cannot open 'treasure.txt'");
        return 0;
    }

    while (my_fgets(line, N, fd) != NULL) {
        char *treasureID = strtok(line, ",");
        if (treasureID && !strcmp(treasureID, treasure)) {
            close(fd);
            return 1; // Treasure exists
        }
    }

    close(fd);
    return 0; // Treasure does not exist
}

int main(int argc, char** argv){
    struct stat st;

    if(!strcmp(argv[1], "-add")){
        //if already the directory already exists, write in the file treasure.bin the treasure, else create the Dir and Files and after that implement the treasure
        if(chdir("hunts")!=0){
            perror("Cannot access the directory");
            return 1;
        }
        if(stat(argv[2], &st)==0&&S_ISDIR(st.st_mode)){
            if(chdir(argv[2])!=0){
                perror("Cannot access the directory");
                return 1;
            }
            int fl=open("logs.txt", O_WRONLY|O_APPEND);
            for(int i=3; i<argc; i++){
                int fd=open("treasure.txt",O_WRONLY|O_APPEND);
                writeInTreasureFile(fd, fl, argv[i]);
                close(fd);
            }
            close(fl);
        }else{
            if(mkdir(argv[2], 0755)==-1){
                perror("Somehow doesn't create the directory");
                return 1;
            }
            if(chdir(argv[2])!=0){
                perror("Cannot access the directory");
            }
            if(creat("treasure.txt", 0644)==-1){
                perror("creat failed");
                return 1;
            }
            if(creat("logs.txt", 0644)==-1){
                perror("creat failed");
                return 1;
            }

            int fl=open("logs.txt", O_WRONLY|O_APPEND);
            char line[256]="";
            sprintf(line, "%s was created and also treasure.txt and log.txt.\n", argv[2]);
            write(fl, line, strlen(line));
                perror("Cannot write");
            for(int i=3; i<argc; i++){
                int fd=open("treasure.txt",O_WRONLY|O_APPEND);
                writeInTreasureFile(fd,fl, argv[i]);
                close(fd);
            }
            close(fl);

        }

    }
    else if(!strcmp(argv[1], "-list")){
        struct stat st;
        //enter in hunts directory
        if(chdir("hunts")!=0){
            perror("Cannot access the directory");
            return 1;
        }
        //check if the directory exists
        if(stat(argv[2], &st)==0&&S_ISDIR(st.st_mode)){
            if(chdir(argv[2])!=0){
                perror("Cannot access this directory or it doesn't exist");
                return 1;
            }
            //compute the status of file treasure.txt
            struct stat fileStat;
            stat("treasure.txt", &fileStat);
            struct tm* lastModTime=localtime(&fileStat.st_mtime);
            //list general things+all content of treasure.txt 
            printf("%s, %ld bytes, last modification: %02d.%02d.%02d %02d:%02d:%02d\n", argv[2], st.st_size,lastModTime->tm_mday,lastModTime->tm_mon+1, lastModTime->tm_year+1900,lastModTime->tm_hour, lastModTime->tm_min, lastModTime->tm_sec);
            printf("----------------------------------------------------------------\n");
            int fd=open("treasure.txt", O_RDONLY);
            char line[N];
            while(my_fgets(line, N, fd)!=NULL){
                printf("%s", line);
            }
            close(fd);            
        }   

        else {
            perror("The directory doesn't exist or the name is wrong");
            return 1;
        }
    }
    else if (!strcmp(argv[1], "-view")) {
        // Check if the "hunts" directory exists
        struct stat st;
        if (stat("hunts", &st) != 0 || !S_ISDIR(st.st_mode)) {
            perror("The 'hunts' directory does not exist");
            return 1;
        }

        // Enter the "hunts" directory
        if (chdir("hunts") != 0) {
            perror("Cannot access the 'hunts' directory");
            return 1;
        }

        // Check if the specified hunt directory exists
        if (stat(argv[2], &st) != 0 || !S_ISDIR(st.st_mode)) {
            fprintf(stderr, "The specified hunt directory '%s' does not exist or is not a directory\n", argv[2]);
            return 1;
        }

        // Enter the specified hunt directory
        if (chdir(argv[2]) != 0) {
            perror("Cannot access the specified hunt directory");
            return 1;
        }

        // Check if the treasure exists
        if (theTreasureExists(argv[3])) {
            int fd = open("treasure.txt", O_RDONLY);
            if (fd == -1) {
                perror("Cannot open 'treasure.txt'");
                return 1;
            }

            char line[N];
            Treasure t;
            while (my_fgets(line, N, fd) != NULL) {
                char *p;
                int contor = 0;
                p = strtok(line, ",");
                if (!strcmp(p, argv[3])) {
                    p = strtok(NULL, ",");
                    while (p) {
                        switch (contor) {
                            case 0: { strcpy(t.userName, p); break; }
                            case 1: { t.latitude = atof(p); break; }
                            case 2: { t.longitude = atof(p); break; }
                            case 3: { strcpy(t.clue, p); break; }
                            case 4: { t.value = atoi(p); break; }
                            default: { break; }
                        }
                        contor++;
                        p = strtok(NULL, ",");
                    }

                    printf("User: %s, latitude: %f, longitude: %f, clue: %s, value: %d;\n",
                           t.userName, t.latitude, t.longitude, t.clue, t.value);
                    break; // Exit loop after finding the treasure
                }
            }
            close(fd);
        } else {
            fprintf(stderr, "The treasure '%s' does not exist in the hunt '%s'\n", argv[3], argv[2]);
            return 1;
        }
    }
    else if(!strcmp(argv[1], "-remove_treasure")){

    }
    else if(!strcmp(argv[1], "-remove_hunt")){

    }

    return 0;
}