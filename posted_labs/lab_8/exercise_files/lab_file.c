#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <zconf.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include<sys/shm.h>

#define PERM 0666
#define MSGSIZE 100



// write code where parent write to memory, child overwrite it after reading 
// and parent should read again to get the value written by child

int main(void) {

    // generate unique key
    key_t key;
    key = ftok("readme", 6765);

    // creates a message queue and allocate memory
    int msgid;
    if ((msgid = shmget(key, MSGSIZE ,PERM | IPC_CREAT)) == -1) {
        perror("Failed to create a queue\n");
        return -1;
    }

    // attach the shared memory
    void* shm;
    if ((shm=shmat(msgid,NULL,0)) == NULL){
        perror("Unable to attach the memory\n");
        return -1;
    }

    // parent writes to memory
    char* s = "parent";
    strcpy((char*)shm, s);
    printf("Parent wrote: %s\n", (char*)shm);
    
    // fork process
    pid_t pid;
    if((pid = fork()) == -1) {
        perror("Unable to fork");
        return -1;
    }
    else if(pid == 0) {
        // child reads memory
        printf("Child read: %s\n", (char*)shm);

        // child overwrites memory
        char* t = "child";
        strcpy((char*)shm, t);
        printf("Child wrote: %s\n", (char*)shm);

        // child detaches memory
        if(shmdt(shm) == -1) {
            perror("Unable to detach the memory\n");
            return -1;
        }
        
        // finished child process
        printf("Back to the parent...\n");
        return 0;
    }

    // waits for child process
    printf("Waiting for the child...\n");
    if(wait(NULL) == -1) {
        perror("Unable to wait");
        return -1;
    }
    
    // parent reads what child wrote
    printf("Parent read: %s\n", (char*)shm);
    
    // parent detaches memory
    if(shmdt(shm) == -1) {
        perror("Unable to detach the memory\n");
        return -1;
    }

    // parent destroys shared memory
    if(shmctl(msgid, IPC_RMID, NULL) == -1) {
        perror("Unable to destroy the memory\n");
        return -1;
    }
     
    return 0;
}

