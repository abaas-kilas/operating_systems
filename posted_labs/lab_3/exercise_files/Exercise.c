#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
int main()
{
    
    /*
   
    Program Instruction:
    You have 3 executables you need to execute.
    readFile
    readFile2
    mergedCount
    
    
    Write a program that will execute readFile and readFile2 in parallel.
    mergedCount must be executed after the first two executables are done executing.
    
    Note( readFile and readFile2 take in one command line arguement). 
    Pass in file1.txt as an arguement for readFile and pass in file2.txt as an arguement for readFile20
    
    You can check mergedCount.txt after the executables have been executed. It should have all the word that are common between file1.txt and file2.txt
    and it should have their word counts.
    */

    pid_t pid1 = fork();
    int status1;
    if (pid1 == -1){
        perror("fork() failed\n");
        exit(1);
    }
    else if(pid1 == 0){
        execl("./readFile", "readFile", "file1.txt", NULL);
        perror("exec() failed\n");
        exit(1);
    }

    pid_t pid2 = fork();
    int status2;
    if (pid2 == -1){
        perror("fork() failed\n");
        exit(1);
    }
    else if(pid2 == 0){
        execl("./readFile2", "readFile2", "file2.txt", NULL);
        perror("exec() failed\n");
        exit(1);
    }

    wait(&status1);
    wait(&status2);

    pid_t pid3 = fork();
    int status3;
    if (pid3 == -1){
        perror("fork() failed\n");
        exit(1);
    }
    else if(pid3 == 0){
        execl("./mergeCount", "mergeCount", NULL);
        perror("exec() failed\n");
        exit(1);
    }
     
    wait(&status3);
    
    return 0;
}
