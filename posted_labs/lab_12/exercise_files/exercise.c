// dependencies
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

// beat counter, signal set, and interrupt counter
int beats;
struct sigaction act;
int count_int;

// first handler increments heart beat count every 2 seconds
void heartbeat_handler(int signum){
    // ignore (signal safety)
    if(signal(signum, SIG_IGN) == SIG_ERR){
        perror("signal error\n");
        exit(1);
    }
    
    // display heart beat count and reset alarm
    printf("Heart beat recieved --> Count: [%d]\n", ++beats);
    alarm(2);

    // reinstall handler
    if(signal(signum, heartbeat_handler) == SIG_ERR){
        perror("signal error\n");
        exit(1);
    }
    
    return;
}

// handler two manages interrupts and needs confirmation before exiting
void ctrlc_handler(int signum){
    // ignore (signal safety)
    if(signal(signum, SIG_IGN) == SIG_ERR){
        perror("signal error\n");
        exit(1);
    }
    
    // manage interrupts and confirmation
    if(++count_int > 1){
        printf(" --> Confirmed! Exiting now\n");
        exit(0);
    }
    else{
        printf(" --> Interupt recieved. Repeat to confirm exit\n");
    }

    // reinstall handler
    if(sigaction(signum, &act, NULL) == -1){
        perror("sigaction error\n");
        exit(1);
    }

    return;
}

// main starts the program
int main(int argc, char *argv[]){
    printf("Program Starting...\n");
    
    // inits
    beats = 0;
    count_int = 0;

    // install first handler
    if(signal(SIGALRM, heartbeat_handler) == SIG_ERR){
        perror("signal error\n");
        exit(1);
    }

    // install second handler
    act.sa_handler = ctrlc_handler;
    act.sa_flags = 0;
    sigemptyset((sigset_t *) &act.sa_flags);
    if(sigaction(SIGINT, &act, NULL) == -1){
        perror("sigaction error\n");
        exit(1);
    }
    
    // give first handler initial alarm and loop so program stays running
    alarm(2);
    while(1){}
    
    printf("Program Exiting...\n");
    return EXIT_SUCCESS;
}