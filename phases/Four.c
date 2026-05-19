#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// This file will explicitly handle the case when there's a pipe operator present!
int main()
{
    while(true) {
        // Taking input from the user!
        char userInput[1024];
        printf("$ ");
        fgets(userInput, sizeof(userInput), stdin);
        userInput[strcspn(userInput, "\n")] = '\0';
    
        // Checking for an empty input!
        if(strlen(userInput) == 0) {
            continue;
        }

        // Checking if user has typed "exit"!
        if(strcmp(userInput, "exit") == 0) {
            printf("Successfully exited the terminal\n");
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
        
        // Setting up the pipe, to pass input's and output's across processes!
        int pipeDescriptors[2];
        if(pipe(pipeDescriptors) == -1) {
            printf("Error: Could not set up the pipe!\n");
            continue;
        }

        // We'll create two children - first child will handle the first command, and the second child will handle the second command!
        int firstID = fork();
        char *firstCommand[1024]; // For the first execvp() command!
        char *secondCommand[1024]; // For the second execvp() command!
        int pipeIndex = -1;

        // Setting up the two command arrays!
        int currentIndex = 0;
        while(currentIndex < count && strcmp(portions[currentIndex], "|") != 0) {
            currentIndex++;
        }
        
        // else {
            pipeIndex = currentIndex;
            int settingIndex = 0; // To set up the first and second command vectors!
            while(settingIndex != pipeIndex) {
                firstCommand[settingIndex] = portions[settingIndex];
                settingIndex++;
            }
            firstCommand[settingIndex] = NULL;
    
            settingIndex = pipeIndex + 1;
            while(settingIndex != count) {
                secondCommand[settingIndex - pipeIndex - 1] = portions[settingIndex];
                settingIndex++;
            }
            secondCommand[settingIndex - pipeIndex - 1] = NULL;
        // }

        // Handling the first command!
        if(firstID == 0) {
                // Closing the read end, since this child won't be reading anything from the pipe!
                close(pipeDescriptors[0]);

                // Changing stdout's file descriptor!
                int oldOutput = dup(STDOUT_FILENO);
                int newOutput = dup2(pipeDescriptors[1], STDOUT_FILENO);
                close(pipeDescriptors[1]);

                // Running the first command!
                int error = execvp(firstCommand[0], firstCommand);
                if(error == -1) {
                    // Restoring stdout for this child, since writing to pipe anyway failed!
                    int restore = dup2(oldOutput, STDOUT_FILENO);
                    close(oldOutput);
                    printf("Error: Could not write to the pipe!\n");
                    exit(1);
                }
        }
        else {
            // Setting up the second child to handle reading from the pipe!
            int secondID = fork();
            if(secondID == 0) {
                // Closing write, since this child doesn't write to the pipe!
                close(pipeDescriptors[1]);

                // Changing stdin's file descriptor!
                int oldInput = dup(STDIN_FILENO);
                int newInput = dup2(pipeDescriptors[0], STDIN_FILENO);
                close(pipeDescriptors[0]);

                // Running the second command!
                int error = execvp(secondCommand[0], secondCommand);
                if(error == -1) {
                    printf("Error: Could not read from the pipe!\n");
                    exit(1);
                }
            }
            else {
                // Closing the descriptor's for the parent!
                close(pipeDescriptors[0]);
                close(pipeDescriptors[1]);
                
                // Wait for both children!
                wait(NULL);
                wait(NULL);
            }
        }
    }
    return 0;
}