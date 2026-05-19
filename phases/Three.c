#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

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

        // Test print!
        // for(int index = 0; index < 3; index++) {
        //     printf("%s\n", portions[index]);
        // }

        int processID = fork();
        if(processID == -1) {
            printf("Error: Could not fork!\n");
            return 1;
        }
        if(processID == 0) {
            // Finding the exact index of "<"!
            int angular = -1, index = 0;
            while(index < count && strcmp(portions[index], "<") != 0) {
                index++;
            }
            angular = index;

            // If angular = size, then we did not find "<"! So it must be a regular command!
            if(angular >= count - 1) {
                if(execvp(portions[0], portions) == -1) {
                    printf("Error: Could not execute %s!\n", portions[0]);
                    exit(1);
                }
            }

            // Otherwise, we've found "<" and can re-direct!
            else {
                // Copying over everything before "<" for the execvp command!
                char *beforeAngular[1024];
                index = 0;
                while(index != angular) {
                    beforeAngular[index] = portions[index];
                    index++;
                }
                beforeAngular[index] = NULL;
    
                // Changing the file descriptor for stdin!
                int currentFile = open(portions[angular + 1], O_RDONLY, 0777);
                int newFile = dup2(currentFile, STDIN_FILENO);
                close(currentFile);
    
                // Running the execvp() command!
                if(execvp(beforeAngular[0], beforeAngular) == -1) {
                    printf("Error: Could not execute %s!\n", beforeAngular[0]);
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