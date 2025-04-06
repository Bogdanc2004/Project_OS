#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define N 10000
#define D 200
typedef struct {
    char treasureID[D];
    char userName[D];
    float latitude;
    float longitude;
    char clue[D];
    int value;

}Treasure;

void writeInTreasureFile(int fd, char *treasure){
    Treasure t;
    char line[N]="";
    printf("Now, let's add the treasure: %s\n", treasure);
    printf("Tresure id: "); scanf("%s", t.treasureID);  printf("\n");
    printf("User: "); scanf("%s", t.userName);  printf("\n");
    printf("Latitude: "); scanf("%f", &t.latitude);  printf("\n");
    printf("Longitude: "); scanf("%f", &t.longitude); printf("\n");
    printf("Clue: "); scanf("%s", t.clue); printf("\n");
    printf("Value:" ); scanf("%d", &t.value); printf("\n");
    sprintf(line, "%s, %s, %s, %f, %f, %s, %d\n",treasure, t.treasureID, t.userName, t.latitude, t.longitude, t.clue, t.value);
    if(write(fd, line, N)==-1)
        perror("Cannot write");
}


int main(int argc, char** argv){
    struct stat st;

    if(!strcmp(argv[1], "-add")){
        //if already the directory already exists, write in the file treasure.bin the treasure, else create the Dir and Files and after that implement the treasure
        if(stat(argv[2], &st)==0&&S_ISDIR(st.st_mode)){

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
            if(creat("hunt.txt", 0644)==-1){
                perror("creat failed");
                return 1;
            }
            for(int i=3; i<argc; i++){
                int fd=open("treasure.txt",O_WRONLY);
                writeInTreasureFile(fd, argv[i]);
                close(fd);
            }
        }

    }
    else if(!strcmp(argv[1], "-list")){

    }
    else if(!strcmp(argv[1], "-view")){

    }
    else if(!strcmp(argv[1], "-remove_treasure")){

    }
    else if(!strcmp(argv[1], "-remove_hunt")){

    }

    return 0;
}