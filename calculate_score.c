#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define N 10000
#define D 200
#define READ_BUF_SIZE 128

typedef struct node{
    char name[D];
    int value;
    struct node* next;
    
}User;

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

User* add(User* head, char name[D], int value){
    User* e=(User*)malloc(sizeof(User));
    strcpy(e->name, name);
    e->value=value;
    e->next=NULL;
    if(head==NULL)
        return e;
    User* curr=head;
    while(curr->next!=NULL)
        curr=curr->next;
    curr->next=e;    
    return head;
}

void freeMem(User* head){
    while(head!=NULL){
        User* p=head;
        head=head->next;
        free(p);
    }
}

User* searchByNameAndSumUp(User* head, char* name, int value){
    User *curr=head;
    while(curr!=NULL){
        if(!strcmp(curr->name, name)){
            curr->value=curr->value+value;
            break;
        }
        curr=curr->next;
    }
    if(curr==NULL)
        head=add(head, name, value);
    return head;
}

User* readFromFile(User* head, char* fileName){
    int fd=open(fileName, O_RDONLY);
    if(fd<0){
        perror("Failed to open file");
        return head;
    }
    char name[D];
    int value;
    char line[N];
    while(my_fgets(line, N, fd)!=NULL){
        char *p=strtok(line, ",\n");
        int contor=0;
        while(p){
            if(contor==1){
                strcpy(name, p);
            }
            else if(contor==5){
                value=atoi(p);
            }
            contor++;
            p=strtok(NULL, ",\n");
        }
        head=searchByNameAndSumUp(head, name, value);
    }
    close(fd);
    return head;
}

void print(User* head, int fp){
    User* curr=head;
    while(curr!=NULL){
        dprintf(fp,"User: %s --- Score: %d\n", curr->name, curr->value);
        curr=curr->next;
    }
}

int main(int argc, char** argv){
    User* head=NULL;
    head=readFromFile(head, argv[2]);
    int fp=atoi(argv[1]);
    print(head, fp);
    freeMem(head);
    close(fp);
    exit(0);
}