#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <ctype.h>

typedef struct pidInfo
{
    int FD;
    char filename[256];
    int inode;
    struct pidInfo* next;
    
}FD;

typedef struct pidInfo_link
{
    pid_t pid;
    FD *fd;
    struct pidInfo_link* next;
}pidLink;

pidLink* newpid(pid_t pid){
    pidLink *node = NULL;
    node = malloc(sizeof(pidLink));
    node->pid = pid;
    node->fd = NULL;
    node->next = NULL;
    return node;   // new pid
}

FD *newfd(){
    FD *node = NULL;
    node = malloc(sizeof(FD));
    node->FD = -1;
    node->inode = -1;
    strcpy(node->filename,"");
    node->next = NULL;
    return node;        // new fd
}

// check the input string can be convert to integer or not
bool validInt(char *s){
    int i = 0;
    while (s[i] != '\0') {
        if (isdigit(s[i]) == 0) {
            return false;  // string s is not a valid integer
        }
        i++;
    }
    return true; // true if the string is a valid integer
}

pidLink* findPid(pid_t pid, pidLink* head){
    pidLink *p = NULL;
    p = head;
    while (p->pid != pid)
    {
        p = p->next;          // find the pid position
    }
    return p;
}

// used to free FD linked list
void deleteFD(pidLink *node){
    FD *p = NULL;
    FD *q = NULL;
    p = node->fd;
    while (p != NULL)
    {
        q = p->next;      // free each FD link
        free(p);
        p = q;
    }
}

// free PID linked list
void deletePID(pidLink *head){
    pidLink *p = NULL;
    pidLink *q = NULL;
    p = head;
    while (p != NULL)
    {
        q = p->next;
        deleteFD(p);          // free FD linked list
        free(p);              // free PidLink
        p = q;
    }
}

// the number of FD in each PID
int countFD(FD *node){
    FD *p = NULL;
    p = node;
    int count = 0;
    while (p != NULL)
    {
        count ++;
        p = p->next;
    }
    return count;
}

// insert FD linked list to node->fd
pidLink *insertFD(pid_t pid, pidLink *node){
    char dirname[256];
    snprintf(dirname,sizeof(dirname),"/proc/%d/fd",pid);
    DIR *dir = opendir(dirname);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL){          // FD
        if(validInt(entry->d_name)){
            char fdlink[256];
            snprintf(fdlink,sizeof(fdlink),"/proc/%d/fd/%d",pid,atoi(entry->d_name));
            char filename[256];
            int len = readlink(fdlink, filename, sizeof(filename)-1); // in <unistd.h>
            filename[len] = '\0';            // filename
            struct stat sb;
            lstat(fdlink, &sb);
            int inode = sb.st_ino;         // inode
            FD *fd = newfd();              // new fd
            fd->FD = atoi(entry->d_name);     // update FD
            fd->inode = inode;             // update inode
            strcpy(fd->filename,filename);  // update filename

            if(node->fd == NULL){
                node->fd = fd;
            }else{
                FD *current = node->fd;
                while (current->next != NULL) {
                    current = current->next; // Find the last node in the list
                }
                current->next = fd; // Append the new node
            }
        }
    }
    closedir(dir);
    return node;
}

// insert pid
pidLink *insertpid(pid_t pid, pidLink *head){
    pidLink *newnode = newpid(pid);
    newnode = insertFD(pid,newnode);
    if (head == NULL) {
        head = newnode;
    } else {
        pidLink* current = head;
        while (current->next != NULL) {
            current = current->next; // Find the last node in the list
        }
        current->next = newnode; // Append the new node
    }
    return head;
}

// --per-process display
void perprocess(pid_t pid, pidLink *head, bool flag){
    if(!flag) return;
    if(pid != -1){
        pidLink *target = findPid(pid,head);
        printf("    PID     FD      \n");
        printf("    =============\n");
        FD * current_fd = target->fd;
        while (current_fd != NULL)
        {
            printf("    %d      %d      \n", target->pid, current_fd->FD); // only print indicated pid 
            current_fd = current_fd->next;
        }
    }else{
        printf("            PID     FD   \n");
        printf("    =====================\n");
        int i = 0;
        pidLink *p = NULL;
        p = head;
        while (p != NULL)
        {
            FD * current_fd = p->fd;
            while (current_fd != NULL)
            {
                printf("    %d      %d      %d     \n", i, p->pid, current_fd->FD);  // print all pid
                i++;
                current_fd = current_fd->next;
            }
            p = p->next;
        }
    }
    printf("    =================\n\n");
}

// --systemWide display
void systemWide(pid_t pid, pidLink *head, bool flag){
    if(!flag) return;
    if(pid != -1){
        pidLink *target = findPid(pid,head);
        printf("    PID     FD      Filename \n");
        printf("    =========================\n");
        FD * current_fd = target->fd;
        while (current_fd != NULL)
        {
            printf("    %d      %d      %s\n", target->pid, current_fd->FD, current_fd->filename); // only print indicated pid 
            current_fd = current_fd->next;
        }
    }else{
        printf("            PID     FD      Filename\n");
        printf("    ===================================\n");
        int i = 0;
        pidLink *p = NULL;
        p = head;
        while (p != NULL)
        {
            FD * current_fd = p->fd;
            while (current_fd != NULL)
            {
                printf("    %d      %d      %d      %s\n", 
                i, p->pid, current_fd->FD, current_fd->filename);  // print all pid
                i++;
                current_fd = current_fd->next;
            }
            p = p->next;
        }
    }
    printf("    ============================\n\n");
}

// --vnode display 
void vnode(pid_t pid, pidLink *head, bool flag){
    if(!flag) return;
    if(pid != -1){
        pidLink *target = findPid(pid,head);
        printf("    FD      Inode\n");
        printf("    =============\n");
        FD * current_fd = target->fd;
        while (current_fd != NULL)
        {
            printf("    %d      %d      \n", current_fd->FD, current_fd->inode); // only print indicated pid
            current_fd = current_fd->next;
        }
    }else{
        printf("           FD      Inode\n");
        printf("    ========================\n");
        int i = 0;
        pidLink *p = NULL;
        p = head;
        while (p != NULL)
        {
            FD * current_fd = p->fd;
            while (current_fd != NULL)
            {
                printf("    %d      %d      %d\n", i, current_fd->FD, current_fd->inode); // print all
                i++;
                current_fd = current_fd->next;
            }
            p = p->next;
        }
    }
    printf("    =============\n\n");
}

// --composite display 
void composite(pid_t pid, pidLink *head,bool flag){
    if(!flag) return;
    if(pid != -1){
        pidLink *target = findPid(pid,head);
        printf("    PID     FD      Filename                Inode\n");
        printf("    ===============================================\n");
        FD * current_fd = target->fd;
        while (current_fd != NULL)
        {
            printf("    %d      %d\t    %s              %d\n", 
            target->pid, current_fd->FD, current_fd->filename, current_fd->inode); // only print indicated pid
            current_fd = current_fd->next;
        }
    }else{
        printf("            PID     FD      Filename                Inode\n");
        printf("    =======================================================\n");
        int i = 0;
        pidLink *p = NULL;
        p = head;
        while (p != NULL)
        {
            FD * current_fd = p->fd;
            while (current_fd != NULL)
            {
                printf("    %d      %d      %d\t    %s          %d\n", 
                i, p->pid, current_fd->FD, current_fd->filename, current_fd->inode); // print all
                i++;
                current_fd = current_fd->next;
            }
            p = p->next;
        }
    }
    printf("    ===============================================\n\n");
}

// --output_TXT store composite in text (ASCII)  into  compositeTable.txt
void txtoutput(pid_t pid, pidLink *head, bool flag){
    if(!flag) return;
    FILE *txt = fopen("compositeTable.txt","w");
    if(pid != -1){
        pidLink *target = findPid(pid,head);
        fprintf(txt,"Target Pid: %d\n\n", pid);    // store them into file
        fprintf(txt,"    PID     FD      Filename                Inode\n");  
        fprintf(txt,"    ===============================================\n");
        FD * current_fd = target->fd;
        while (current_fd != NULL)
        {
            fprintf(txt,"    %d      %d\t   %s              %d\n", 
            target->pid, current_fd->FD, current_fd->filename, current_fd->inode); // go through each fd
            current_fd = current_fd->next;
        }
    }else{
        fprintf(txt,"           PID     FD      Filename                Inode\n");
        fprintf(txt,"    =====================================================\n");
        int i = 0;
        pidLink *p = NULL;
        p = head;
        while (p != NULL)
        {
            FD * current_fd = p->fd;
            while (current_fd != NULL)
            {
                fprintf(txt,"    %d      %d      %d\t   %s          %d\n", 
                i, p->pid, current_fd->FD, current_fd->filename, current_fd->inode); // go through each fd
                i++;
                current_fd = current_fd->next;
            }
            p = p->next;  // go through each pidlink node
        }
    }
    fclose(txt);
}

// --output_binary store composite in binary  into  compositeTable.bin
void binaryoutput(pid_t pid, pidLink *head,bool flag){
    if(!flag) return;
    FILE *binary = fopen("compositeTable.bin","wb");
    char title[] = "    PID     FD      Filename                Inode\n";
    char line[] = "    ===============================================\n";
    if(pid != -1){
        pidLink *target = findPid(pid,head);
        char targetpid[1024];
        snprintf(targetpid,sizeof(targetpid),"Target Pid: %d\n\n", pid);
        fwrite(targetpid, sizeof(char), strlen(targetpid), binary);      // write in binary
        fwrite(title, sizeof(char), strlen(title), binary);         // write in binary
        fwrite(line, sizeof(char), strlen(line), binary);             // write in binary
        FD * current_fd = target->fd;
        while (current_fd != NULL)
        {
            fwrite(&target->pid, sizeof(pid_t),1,binary);           // write in binary pid
            fwrite(&current_fd->FD,sizeof(int),1,binary);           // write in binary fd
            fwrite(&current_fd->filename,sizeof(char),strlen(current_fd->filename),binary); // write in binary filename
            fwrite(&current_fd->inode,sizeof(int),1,binary);        // write in binary inode
            current_fd = current_fd->next;
        }
    }else{
        fwrite(title, sizeof(char), strlen(title), binary);         // write in binary
        fwrite(line, sizeof(char), strlen(line), binary);             // write in binary
        int i = 0;
        pidLink *p = NULL;
        p = head;
        while (p != NULL)
        {
            FD * current_fd = p->fd;
            while (current_fd != NULL)
            {
                fwrite(&p->pid, sizeof(pid_t),1,binary);    // write in binary pid
                fwrite(&current_fd->FD,sizeof(int),1,binary); // write in binary fd
                fwrite(&current_fd->filename,sizeof(char),strlen(current_fd->filename),binary); // write in binary filename
                fwrite(&current_fd->inode,sizeof(int),1,binary); // write in binary inode
                i++;
                current_fd = current_fd->next;
            }
            p = p->next;
        }
    }
    fclose(binary);
}

// --threshold display
void threshold(int thresholdNum, pidLink *head,bool flag){
    if(!flag) return;
    char output[1024];
    strcpy(output,"");
    pidLink *p = head;
    while (p != NULL)
    {
        int count = countFD(p->fd);
        if(count > thresholdNum){
            char result[30];
            snprintf(result,sizeof(result),"%d(%d)  ",p->pid,count);
            strcat(output,result);         // get the printing result
        }
        p = p->next;
    }
    printf("## Offending processes:\n");
    printf("%s\n",output);
}

// check each input command line
pid_t checkFlag(int argc, char** argv, bool* processflag, bool* systemWideflag, bool* Vnodesflag, bool* compositeflag, bool* thresholdflag,
    int* thresholdNum, bool* output_TXTflag, bool* output_binaryflag){
        pid_t pid = -1;
        for(int i = 1; i < argc; i ++){
            char *token = strtok(argv[i], "="); // split the input argument by "="
            if(strcmp(token, "--per-process") == 0){  // check --pre-process
                *processflag = true;
            }else if(strcmp(token, "--systemWide") == 0){       // check --systemWide
                *systemWideflag = true;
            }else if((strcmp(token,"--Vnodes") == 0)){      //check --Vnodes
                *Vnodesflag = true;
            }else if((strcmp(token,"--output_TXT") == 0)){   // check --output_TXT
                *output_TXTflag = true;
            }else if((strcmp(token,"--output_binary") == 0)){  // check --output_binary
                *output_binaryflag = true;
            }
            else if(strcmp(token, "--composite") == 0){    // check --composite
                *compositeflag = true;
            }else if ((strcmp(token,"--threshold")) == 0){       // check --threshold
                *thresholdNum = atoi(strtok(NULL, ""));
                *thresholdflag = true;
            }else if(validInt(argv[i])){          // check specific pid indicated
                char fdaccess[256];
                snprintf(fdaccess,sizeof(fdaccess),"/proc/%d/fd",atoi(argv[i]));
                if(access(fdaccess,R_OK) == 0){
                    pid = atoi(argv[i]);
                }else{
                    printf("PID is not exist or not accessable\n");   // not accessable pid
                    pid = -2; 
                    return pid;
                }
            }else{
                printf("Invalid command line, check your input command line\n");     // wrong command line
                pid = -2;
                return pid;
            }
        }
    return pid;
}

int main(int argc, char** argv){
    bool processflag = false;   // default value of those flag
    bool systemWideflag = false;
    bool Vnodesflag = false;
    bool compositeflag = false;
    bool thresholdflag = false;
    int thresholdNum = 0;
    bool output_TXTflag = false;
    bool output_binaryflag = false;
    pid_t specificPid;
    specificPid = checkFlag(argc, argv, &processflag,&systemWideflag,&Vnodesflag,&compositeflag,
        &thresholdflag,&thresholdNum,&output_TXTflag,&output_binaryflag);  // check flag and return the indicated pid

    if (specificPid == -2){  // not accessable or not existing PID command line
        return 1;
    }

    pidLink *head = NULL;
    DIR *dir = opendir("/proc");
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if(validInt(entry->d_name)){
            char fdaccess[256];
            snprintf(fdaccess,sizeof(fdaccess),"/proc/%d/fd",atoi(entry->d_name));
            if (access(fdaccess,R_OK) == 0){
                head = insertpid(atoi(entry->d_name),head);               // fetch information
            }
        }
    }
    closedir(dir);
    // out put display
    if(argc == 1 || (thresholdflag && argc == 2)){
        composite(specificPid,head,true);   // default or with threshold
    }
    if((argc == 2 && specificPid != -1)||(argc == 3 && thresholdflag && specificPid != -1)){  // only indicate PID or with PID and threshold
        printf("Target Pid: %d\n\n", specificPid);
        perprocess(specificPid,head,true);
        systemWide(specificPid,head,true);
        vnode(specificPid,head,true);
        composite(specificPid,head,true);
    }
    if(specificPid != -1 && (processflag || systemWideflag || Vnodesflag || compositeflag)){
        printf("Target Pid: %d\n\n", specificPid);
    }
    perprocess(specificPid,head,processflag);   // --per-process
    systemWide(specificPid,head,systemWideflag);  // --systemWide
    vnode(specificPid,head,Vnodesflag);  // --Vnodes
    composite(specificPid,head,compositeflag);  // --composite
    threshold(thresholdNum,head,thresholdflag);  // --threshold=X
    txtoutput(specificPid,head,output_TXTflag);  // --output_TXT
    binaryoutput(specificPid,head,output_binaryflag);  // --output_binary
    deletePID(head);  // free memory 
    return 0;
}