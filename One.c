#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ncurses.h>

int main(int argumentCount, char *argumentVector[])
{
    initscr();
    while(true) {
        // Taking input from the user!
        char userInput[1024];
        printw("> ");
        getstr(userInput);
        userInput[strcspn(userInput, "\n")] = '\0';
    
        // Checking for an empty input!
        if(strlen(userInput) == 0) {
            continue;
        }

        // Checking if user has typed "exit"!
        if(strcmp(userInput, "exit") == 0) {
            printw("Successfully exited the terminal\n");
            break;
        }

        // Splitting the input about the spaces, and placing each "word" into an array!
        char *portions[1024];
        int count = 0;
        char *token = strtok(userInput, " ");
        while(token != NULL) {
            portions[count] = token;
            token = strtok(NULL, " ");
            count++;
        }
        // Adding the required NULL terminator!
        portions[count] = NULL;
    
        // Calling chdir() for cd!
        if(strcmp(portions[0], "cd") == 0) {
            if(portions[1] == NULL) {
                continue;
            }
            if(chdir(portions[1]) == -1) {
                printw("Error: Could not find path %s\n", portions[1]);
            }
            else {
                printw("Successfully changed directory. Current path - %s\n", portions[1]);
            }
            continue;
        }

        // Setting up the child process to handle the user input!
        int processID = fork();
        if(processID == -1) {
            printw("Error: Could not fork\n");
            return 1;
        }
        if(processID == 0) {
            int error = execvp(portions[0], portions);
            if(error == -1) {
                printw("Error: Could not find program\n");
                exit(1); // In-case of failure, do not go in-to parent process!
            }
        }
        else {
            int waitStatus;
            wait(&waitStatus);
            if(WIFEXITED(waitStatus)) {
                // if(WEXITSTATUS(waitStatus) == 0) {
                //     printw("Successfully executed %s\n", portions[0]);
                // }
                if(WEXITSTATUS(waitStatus) != 0) {
                    printw("Error: Could not execute %s\n", portions[0]);
                }
            }
        }
    }
    endwin();
    return 0;
}