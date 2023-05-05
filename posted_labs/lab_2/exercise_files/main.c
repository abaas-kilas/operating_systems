#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){

     
//  1. Takes an integer argument from the command line and store it in a variable. example. "num".
     // if argument is not provided, print error message
     int n;
     if (argc != 2){
          printf("ERROR: No argument/too many arguments provided\n");
          exit(1);
     }
     else{
          n = atoi(argv[1]);
     }

// 2. Forks two children processes
     // a. First child computes and prints the first Nth terms of Fibonacci Sequence.  
     // and prints out the result and its own identifier ID (process id).
     // Example:   [ ID = 262750] The list of first 5 terms in the Fibonacci Sequence is: 0, 1, 1, 2, 3
     
     
     // b. Second child computes the factorial of Number (1*2*...*N1) and prints out
     // the result and its own identifier ID.
     // Example [ ID = 4076743] Factorial of 5 is 120
     pid_t pid1 = fork();
     if (pid1 == -1){
          perror("Fork Failed\n");
          exit(1);
     }
     else if(pid1 == 0){
          printf("[ID: %d] The list of first %d terms in the Fibonacci Sequence is: ",getpid(),n);
          int f1 = 0, f2 = 1;
          if (n > 0){
               printf("%d ", f1);
               for (int i = 2; i <= n; i++) {
                    printf("%d ", f2);
                    int sum = f1 + f2;
                    f1 = f2;
                    f2 = sum;
               }
          }
          printf("\n");
          exit(0);
     }

     pid_t pid2 = fork();
     if (pid2 == -1){
          perror("Fork Failed\n");
          exit(1);
     }
     else if(pid2 == 0){
          printf("[ID: %d] Factorial of %d is ",getpid(),n);
          int k = 1;
          for (int i = 2; i <= n; i++)
               k *= i;
          printf("%d\n", k);
          exit(0);
     }
     
// 3. Parent waits until both children are finished, then prints out the message “Done”
     // and its own identifier.
     // Example: [ID = 4076741] DONE
     wait(NULL);
     wait(NULL);
     printf("[ID: %d] Done\n",getpid());
     
     return 0;
}
