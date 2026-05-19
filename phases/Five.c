#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int main(int argumentCount, char *argumentVector[])
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
        
        // Calling chdir() for cd!
        if(strcmp(portions[0], "cd") == 0) {
            if(portions[1] == NULL) {
                continue;
            }
            if(chdir(portions[1]) == -1) {
                printf("Error: Could not find path %s\n", portions[1]);
            }
            else {
                printf("Successfully changed directory. Current path - %s\n", portions[1]);
            }
            continue;
        }
        
        // Finding pipe index!
        int pipeIndex = -1;
        int currentIndex = 0;
        while(currentIndex < count && strcmp(portions[currentIndex], "|") != 0) {
            currentIndex++;
        }
        if(currentIndex < count) {
            pipeIndex = currentIndex;
        }
        
        // Handling pipe command!
        if(pipeIndex != -1) {
            // Setting up the pipe, to pass input's and output's across processes!
            int pipeDescriptors[2];
            if(pipe(pipeDescriptors) == -1) {
                printf("Error: Could not set up the pipe!\n");
                continue;
            }
        
            // We'll create two children - first child will handle the first command, and the second child will handle the second command!
            int firstID = fork();
            char *firstCommand[1024];
            char *secondCommand[1024];
        
            // Setting up the two command arrays!
            int settingIndex = 0;
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
            continue;
        }
        
        // Setting up the child process to handle the user input!
        int processID = fork();
        if(processID == -1) {
            printf("Error: Could not fork!\n");
            return 1;
        }
        if(processID == 0) {
            // Finding the exact index of ">"!
            int rightAngular = -1;
            int index = 0;
            while(index < count && strcmp(portions[index], ">") != 0) {
                index++;
            }
            if(index < count) {
                rightAngular = index;
            }
        
            // Finding the exact index of "<"!
            int leftAngular = -1;
            index = 0;
            while(index < count && strcmp(portions[index], "<") != 0) {
                index++;
            }
            if(index < count) {
                leftAngular = index;
            }
        
            // Handling output re-direction!
            if(rightAngular != -1) {
                // Copying over everything before ">" for the execvp command!
                char *beforeAngular[1024];
                index = 0;
                while(index != rightAngular) {
                    beforeAngular[index] = portions[index];
                    index++;
                }
                beforeAngular[index] = NULL;
        
                // Changing the file descriptor for stdout!
                int currentFile = open(portions[rightAngular + 1], O_WRONLY | O_CREAT | O_TRUNC, 0777);
                int newFile = dup2(currentFile, STDOUT_FILENO);
                close(currentFile);
        
                // Running the execvp() command!
                if(execvp(beforeAngular[0], beforeAngular) == -1) {
                    printf("Error: Could not execute %s!\n", beforeAngular[0]);
                    exit(1);
                }
            }
        
            // Handling input re-direction!
            else if(leftAngular != -1) {
                // Copying over everything before "<" for the execvp command!
                char *beforeAngular[1024];
                index = 0;
                while(index != leftAngular) {
                    beforeAngular[index] = portions[index];
                    index++;
                }

                beforeAngular[index] = NULL;

                // Changing the file descriptor for stdin!
                int currentFile = open(portions[leftAngular + 1], O_RDONLY);
                int newFile = dup2(currentFile, STDIN_FILENO);
                close(currentFile);

                // Running the execvp() command!
                if(execvp(beforeAngular[0], beforeAngular) == -1) {
                    printf("Error: Could not execute %s!\n", beforeAngular[0]);
                    exit(1);
                }
            }
        
            // Handling normal commands!
            else {
                int error = execvp(portions[0], portions);
                if(error == -1) {
                    printf("Error: Could not execute %s!\n", portions[0]);
                    exit(1);
                }
            }
        }
        else {
            int waitStatus;
            wait(&waitStatus);
            if(WIFEXITED(waitStatus)) {
                int statusCode = WEXITSTATUS(waitStatus);
                if(statusCode != 0) {
                    printf("Error: Failed to execute %s!\n", portions[0]);
                }
            }
        }
    }
    return 0;
}