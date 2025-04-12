#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <linux/limits.h>
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
    strcpy(t.treasureID, treasure);
    printf("User(String): "); scanf("%s", t.userName); fflush(stdout);  printf("\n");
    printf("Latitude(Float): "); scanf("%f", &t.latitude);fflush(stdout);   printf("\n");
    printf("Longitude(Float): "); scanf("%f", &t.longitude); fflush(stdout); printf("\n");
    printf("Clue(Text): "); getchar(); fgets(t.clue, sizeof(t.clue),stdin); fflush(stdout); printf("\n");
    printf("Value(Integer):" ); scanf("%d", &t.value); fflush(stdout); printf("\n");
    t.clue[strlen(t.clue)-1]='\0';
    sprintf(line, "%s, %s, %f, %f, %s, %d\n",t.treasureID, t.userName, t.latitude, t.longitude, t.clue, t.value);
    if(write(fd, line, strlen(line))==-1){
        perror("Cannot write");
        exit(-1);
    }
    sprintf(line1, "%s was recorded by user %s.\n", treasure, t.userName);
    if(write(fl, line1, strlen(line1))==-1){
        perror("Cannot record in log-file");
        exit(-1);
    }
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
        char lineCopy[N];
        strcpy(lineCopy, line); // Create a copy of the line
        char *treasureID = strtok(lineCopy, ",");
        if (treasureID && !strcmp(treasureID, treasure)) {
            close(fd);
            return 1; // Treasure exists
        }
    }

    close(fd);
    return 0; // Treasure does not exist
}

void getInfoInLog(int fl, int argc, char**argv){
    char line[N]="";
    for(int i=0; i<argc; i++){
        strcat(line, argv[i]);
        strcat(line, " ");
    }
    strcat(line, "\n");
    write(fl, line, strlen(line));
}

void cpyInManager(int fl, char* path, int argc, char**argv){

    int fp = open(path, O_APPEND | O_WRONLY);
    if (fp == -1) {
        perror("Cannot open master log file");
        exit(-1);
    }

    char line[N];
    while (my_fgets(line, N, fl) != NULL) {
        printf("%s\n",line);
        write(fp, line, strlen(line)); 

    }
    getInfoInLog(fp, argc, argv);
    close(fp);
}

void addHuntTreasure(int argc, char** argv){
    struct stat st;
    //if already the directory already exists, write in the file treasure.bin the treasure, else create the Dir and Files and after that implement the treasure
    if(chdir("hunts")!=0){
        perror("Cannot access the directory");
        exit(-1);
    }
    if(stat(argv[2], &st)==0&&S_ISDIR(st.st_mode)){
        if(chdir(argv[2])!=0){
            perror("Cannot access the directory");
            exit(-1);
        }
        int fl=open("logged_hunt.txt", O_WRONLY|O_APPEND);
        getInfoInLog(fl, argc, argv);
        for(int i=3; i<argc; i++){
            int fd=open("treasure.txt",O_WRONLY|O_APPEND);
            writeInTreasureFile(fd, fl, argv[i]);
            close(fd);
        }
        close(fl);
    }else{
        if(mkdir(argv[2], 0755)==-1){
            perror("Somehow doesn't create the directory");
            exit(-1);
        }
        if(chdir(argv[2])!=0){
            perror("Cannot access the directory");
            exit(-1);
        }
        if(creat("treasure.txt", 0644)==-1){
            perror("creat failed");
            exit(-1);
        }
        if(creat("logged_hunt.txt", 0744)==-1){
            perror("creat failed");
            exit(-1);
        }

        int fl=open("logged_hunt.txt", O_WRONLY|O_APPEND);
        getInfoInLog(fl, argc, argv);
        char line[256]="";
        sprintf(line, "%s was created and also treasure.txt and log.txt.\n", argv[2]);
        if(write(fl, line, strlen(line))==-1){
            perror("Cannot write");
            exit(-1);
        }
        for(int i=3; i<argc; i++){
            int fd=open("treasure.txt",O_WRONLY|O_APPEND);
            writeInTreasureFile(fd,fl, argv[i]);
            close(fd);
        }
        
        close(fl);

        // Navigate to the logs directory
        if (chdir("../../logs") != 0) {
            perror("Cannot access the 'logs' directory");
            exit(-1);
        }
        #ifdef DEL
        printf("Current directory after navigating to 'logs': %s\n", getcwd(NULL, 0)); // Debug print
        #endif
        char name[N] = "";
        sprintf(name, "logged_hunt-%s.txt", argv[2]);
        #ifdef DEL
        printf("Created log file: %s\n", name); // Debug print
        #endif
        // Navigate back to the hunt directory
        if (chdir("../hunts") != 0 || chdir(argv[2]) != 0) {
            perror("Cannot navigate back to the hunt directory");
            exit(-1);
        }
        char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd failed");
        exit(-1);
    }

    // Build absolute path to target
    char targetLink[N];
    snprintf(targetLink, sizeof(targetLink), "%s/logged_hunt.txt", cwd);

    // Now go to logs directory
    if (chdir("../../logs") != 0) {
        perror("Cannot access the 'logs' directory");
        exit(-1);
    }

    // Link will be created here
    char linkName[N];
    snprintf(linkName, sizeof(linkName), "logged_hunt-%s.txt", argv[2]);

    // Check if the symbolic link already exists
    if (access(linkName, F_OK) == 0) {
        printf("Symbolic link '%s' already exists. Removing it.\n", linkName); // Debug print
        if (unlink(linkName) == -1) {
            perror("Cannot remove existing symbolic link");
            exit(-1);
        }
    }

    // Create the symbolic link
    if (symlink(targetLink, linkName) == -1) {
        perror("Cannot create the symbolic link");
        exit(-1);
    }
    #ifdef DEB
    printf("Symbolic link created successfully: %s -> %s\n", linkName, targetLink); // Debug print
    #endif
    }
    

}

void list(int argc, char** argv){
    struct stat st;
    //enter in hunts directory
    if(chdir("hunts")!=0){
        perror("Cannot access the directory");
        exit(-1);
    }
    //check if the directory exists
    if(stat(argv[2], &st)==0&&S_ISDIR(st.st_mode)){
        if(chdir(argv[2])!=0){
            perror("Cannot access this directory or it doesn't exist");
            exit(-1);
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
        int fl=open("logged_hunt.txt", O_APPEND|O_WRONLY);
        getInfoInLog(fl, argc, argv);
        close(fl);
        close(fd);            
    } else {
        perror("The directory doesn't exist or the name is wrong");
        exit(-1);
    }
}

void viewTreasure(int argc, char**argv){
    // Check if the "hunts" directory exists
    struct stat st;
    if (stat("hunts", &st) != 0 || !S_ISDIR(st.st_mode)) {
        perror("The 'hunts' directory does not exist");
        exit(-1);
    }

    // Enter the "hunts" directory
    if (chdir("hunts") != 0) {
        perror("Cannot access the 'hunts' directory");
        exit(-1);
    }

    // Check if the specified hunt directory exists
    if (stat(argv[2], &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "The specified hunt directory '%s' does not exist or is not a directory\n", argv[2]);
        exit(-1);
    }

    // Enter the specified hunt directory
    if (chdir(argv[2]) != 0) {
        perror("Cannot access the specified hunt directory");
        exit(-1);
    }

    int fd = open("treasure.txt", O_RDONLY);
    if (fd == -1) {
        perror("Cannot open 'treasure.txt'");
        exit(-1);
    }

    char line[N];
    Treasure t;
    int found = 0; // Flag to check if any treasures are found
    printf("---%s---\n", argv[3]);
    while (my_fgets(line, N, fd) != NULL) {
        char lineCopy[N];
        strcpy(lineCopy, line); // Create a copy of the line
        char *p = strtok(lineCopy, ",");
        if (p && !strcmp(p, argv[3])) {
            found = 1;
            int contor = 0;
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
        }
    }

    if (!found) {
        fprintf(stderr, "The treasure '%s' does not exist in the hunt '%s'\n", argv[3], argv[2]);
    }

    int fl = open("logged_hunt.txt", O_APPEND | O_WRONLY);
    getInfoInLog(fl, argc, argv);
    close(fl);
    close(fd);
}

void delHunt(int argc, char** argv){
    // access hunts directory
    struct stat st;
    if(chdir("hunts")!=0){
        perror("Cannot access the directory");
        exit(-1);
    }
    //make sure that the hunt exists
    if(stat(argv[2], &st)==0&&S_ISDIR(st.st_mode)){
        //in order to remove the directory we need to remove the files first
        if(chdir(argv[2])!=0){
            perror("Cannot access the directory");
            exit(-1);
        }
        int fl=open("logged_hunt.txt", O_RDONLY);
        int fp = open("../../masterLog.txt", O_APPEND | O_WRONLY);
        if (fp == -1) {
            perror("Cannot open master log file");
            exit(-1);
        }
    
        char line[N];
        char line1[N];
        sprintf(line1, "%s\n", argv[2]);
        write(fp, line1, strlen(line1));
        
        while (my_fgets(line, N, fl) != NULL) {
            write(fp, line, strlen(line)); 
    
        }
        getInfoInLog(fp, argc, argv);
        close(fp);
        close(fl);
        unlink("treasure.txt");
        unlink("logged_hunt.txt");
        chdir("..");
        if(rmdir(argv[2])!=0){
            perror("Cannot remove the hunt");
            exit(-1);
        }
        if(chdir("../logs")!=0){
            perror("Cannot access logs!");
            exit(-1);
        }
        char linkedFile[N]="logged_hunt-";
        strcat(linkedFile, argv[2]);
        strcat(linkedFile, ".txt");
        unlink(linkedFile);

    }else{
        perror("This directory doesn't exist");
        exit(-1);
    }

}

void delRecordsOfTreasureAndUpdate(int argc, char**argv){
    struct stat st;
    //enter hunts
    if(chdir("hunts")!=0){
        perror("Cannot access the directory");
        exit(-1);
    }
    if(stat(argv[2], &st)==0&&S_ISDIR(st.st_mode)){
        //enter hunt 
        if(chdir(argv[2])==0){
            //create a temp file for records!=treasure 
            if(creat("temp.txt", 0655)==-1){
                perror("Couldn't create the file");
                exit(-1);
            }
            int fd=open("treasure.txt", O_RDONLY);
            int ft=open("temp.txt", O_APPEND|O_WRONLY);
            char line[N];
            char line1[N];
            while(my_fgets(line, N, fd)!=NULL){
                strcpy(line1, line);
                if(strcmp(argv[3], strtok(line, ","))){
                    write(ft, line1, strlen(line1));
                }
            }
            close(fd);
            close(ft);
            ft=open("temp.txt", O_RDONLY);
            fd=open("treasure.txt", O_WRONLY| O_CREAT| O_TRUNC, 0644);
            while(my_fgets(line, N, ft)!=NULL){
                write(fd, line, strlen(line));
            }
            close(ft);
            int fl=open("logged_hunt.txt", O_APPEND|O_WRONLY);
            getInfoInLog(fl, argc, argv);
            close(fl);
            close(fd);
            unlink("temp.txt");
        }
        else {
            perror("Cannot access the hunt");
            exit(-1);
        }
    }else{
        perror("The hunt doesn't exist");
        exit(-1);
    } 
}

int main(int argc, char** argv){
    
    if(!strcmp(argv[1], "--add")){
        addHuntTreasure(argc, argv);
    }
    else if(!strcmp(argv[1], "--list")){
       list(argc, argv);
    }
    else if (!strcmp(argv[1], "--view")) {
      viewTreasure(argc, argv);
    }
    else if(!strcmp(argv[1], "--remove_treasure")){
        delRecordsOfTreasureAndUpdate(argc, argv);
    }
    else if(!strcmp(argv[1], "--remove_hunt")){    
        delHunt(argc, argv);
    }
    else {
        perror("Wrong input"); 
        printf("You have these options:\n --add <hunt_id> <treasure_id> (Optional more <treasure_id>'s)\n --list <hunt_id>\n");
        printf(" --view <hunt_id> <treasure_id>\n --remove_treasure <hunt_id> <treasure_id>\n --remove_hunt <hunt_id>\n");
        return 1;
    }
    return 0;
}