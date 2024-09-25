#include <sys/time.h>
#include  <stdbool.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct {
    char *command;
    int pid;
    struct timeval time;
    long duration; // in microseconds
}Command ;

#define n 1024 //max length of a command

Command history[100]; //array to store commands
int curcommands = 0; //number of commands executed

void storecmd(char *cmd, int pid, struct timeval start, long duration) {
    //Funtion to store command.
    if (curcommands < 100) {
        history[curcommands].command = strdup(cmd);
        history[curcommands].pid = pid;
        history[curcommands].time = start;
        history[curcommands].duration = duration;
        curcommands++;
    }
}

void printhistory(bool deets) {
    //Funtion to print history
    for (int i = 0; i < curcommands; i++) {
        struct timeval start = history[i].time;
        long seconds = start.tv_sec + 5*3600 + 30*60;
        long useconds = start.tv_usec; 
        seconds = seconds%86400;
        long hours=seconds/3600;
        seconds=seconds-hours*3600;
        long minutes=seconds/60;
        seconds=seconds-minutes*60;
         
        printf("Command: %s\n", history[i].command);
        if(deets){ // when exiting
            printf("Program ID: %d\n", history[i].pid);
            printf("Time: %ld Hours and %ld minutes and %ld seconds and %ld microseconds\n", hours,minutes,seconds, useconds);
            printf("Duration: %ld microseconds\n", history[i].duration);
            printf("\n");
        }
    }
}

void handle_ctrlc(int sig) {
    // Signal handler for Ctrl+C (SIGINT)
    printhistory(true); // Show command history with details
    exit(0); // Exit the program
}


int main() {
    
    char fullcmd[n]; //array to store current command.
    signal(SIGINT, handle_ctrlc);

    do {
        printf("AS'S> ");
        fgets(fullcmd, n, stdin);
        // Remove newline character from the command
        fullcmd[strcspn(fullcmd, "\n")] = '\0';
        
        char *copy = strdup(fullcmd); //copy command to store because strtok changes original.
        struct timeval start, end;
        gettimeofday(&start, NULL);
        long duration;
        int pid;

        if (strcmp(fullcmd, "exit") == 0) {
            // Show the command history with details
            printhistory(true);
            break;
        }
        else if (strcmp(fullcmd, "history") == 0) {
            // Show history
            pid=getpid();
            printhistory(false);
            
        }
        else{
            int commands=0; //total number of commands in the full command, will be >1 in case of pipe(s).

            char* cmds[100]; //to store each command in the full command.
            for(char *c = strtok(fullcmd, "|"); c!=NULL; c=strtok(NULL, "|")){
                cmds[commands]=c;
                commands++;
            }
            int k=0;
            char *andcopy = strdup(cmds[commands-1]); //duplicate last commmand
            char *andcheck[100]; //to store the last command and its arguments
            for(char *p = strtok(andcopy," "); p != NULL; p = strtok(NULL, " ")) {
                andcheck[k] = p;
                k++;
            }
            bool check=false;
            if(strcmp(andcheck[k-1], "&") == 0){ //if last command has "&", the whole command needs to run in background
                check=true;
            }

            int prev=-1;
            for(int i=0;i<commands;i++){ // going through each commands seperately 
                int fd[2];
                pipe(fd);
                char *c=cmds[i];
                char *args[100];

                // Parse the command and its arguments
                int j = 0;
                for (char *p = strtok(c," "); p != NULL; p = strtok(NULL, " ")) { // creating array with words seperately of the commands
                    args[j++] = p;           
                }
                if(i==commands-1 && strcmp(args[j-1], "&") == 0){ //if current command has "&"
                    args[j-1]=NULL;
                }

                args[j] = NULL;  // NULL-terminate the array

                // Create a child process to execute the command
                pid = fork();

                int status;
                if (pid == 0) {  // child process 
                    if(prev!=-1){   //if not first take input from pipe
                        dup2(prev,0);
                        close(prev);
                    }
                    if(i!=commands-1){  // if not last write to pipe
                        dup2(fd[1],1);
                        close(fd[1]);
                    }
                    // Child process: execute the command
                    if (execvp(args[0], args) == -1) {
                        perror("Command issue\n");
                    }
                    exit(EXIT_FAILURE);
                } 
                else if (pid > 0) { // parent process
                    if (prev != -1) {   // close prev if not first
                    
                        close(prev);
                    }
                    close(fd[1]);  
                    prev = fd[0];
                if(!check){
                    wait(&status);}
                } else {
                    
                    perror("Couldn't fork");
                }
            }
        }
        gettimeofday(&end, NULL);

        
        duration = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);  // calculating durating in secs and microsecs

        
        storecmd(copy, pid, start, duration);
    }while(1);


    return 0;
}
