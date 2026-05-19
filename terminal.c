#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ncurses.h>
#define bufferSize 4096
#define terminalBufferSize 65536

char terminalBuffer[terminalBufferSize];

void displayOutput(int fileDescriptor) {
    char buffer[bufferSize];
    ssize_t bytesRead;
    while((bytesRead = read(fileDescriptor, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        strncat(terminalBuffer, buffer, terminalBufferSize - strlen(terminalBuffer) - 1);
    }
}

void redrawScreen() {
    clear();

    // Heading for the terminal!
    attron(A_BOLD);
    mvprintw(0, 1, "Terminix");
    attroff(A_BOLD);

    // Adds a horizontal line below the name of the program!
    mvhline(1, 0, ACS_HLINE, COLS);

    // Future improvement!
    // int maximumOutputLines = LINES - 5;

    int startingRow = 3;

    mvprintw(startingRow, 0, "%s", terminalBuffer);
    mvhline(LINES - 2, 0, ACS_HLINE, COLS);
    mvprintw(LINES - 1, 0, "$ ");
    move(LINES - 1, 2);
    refresh();
}

int main(int argumentCount, char *argumentVector[])
{
    initscr();

    // We'll be using a buffer to store old outputs!
    terminalBuffer[0] = '\0';

    // Setting up a theme - Black back-ground and blue fore-ground!
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    wbkgd(stdscr, COLOR_PAIR(1));
    clear();
    refresh();

    cbreak();

    // May-be add noecho() here!
    noecho();

    keypad(stdscr, TRUE); // This enables special keys! 
    scrollok(stdscr, TRUE); // This will allow us to scroll! 
    while(true) {
        // Taking input from the user!
        char userInput[1024];
        
        // Moving the cursor to the approprate location at the start every-time!
        redrawScreen();

        move(LINES - 1, 2);
        clrtoeol();

        // To temporarily see what I'm typing!
        echo();
        getnstr(userInput, sizeof(userInput) - 1);
        noecho();
        
        // "userInput[strcspn(userInput, "\n")] = '\0';" is no longer required, getnstr() handles the new-line terminator!

        // Updating the buffer after taking in input!
        char temporary[2048];
        snprintf(temporary, sizeof(temporary), "$ %s\n", userInput);
        strcat(terminalBuffer, temporary);

        // Checking for an empty input!
        if(strlen(userInput) == 0) {
            continue;
        }

        // Checking if user has typed "exit"!
        if(strcmp(userInput, "exit") == 0) {
            strcat(terminalBuffer, "Successfully exited the terminal!\n");
            redrawScreen();
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
                strcat(terminalBuffer, "Error: Could not find path!\n");
            }
            else {
                strcat(terminalBuffer, "Successfully changed directory!\n");
            }
            continue;
        }
        
        // Handling clear command!
        if(strcmp(portions[0], "clear") == 0) {
            terminalBuffer[0] = '\0';
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
                strcat(terminalBuffer, "Error: Could not set up the shell pipe!\n");
                continue;
            }
        
            // We'll have to create another pipe, one that captures the output from the child and re-directs it to the parent process!
            int outputPipe[2];
            if(pipe(outputPipe) == -1) {
                strcat(terminalBuffer, "Error: Could not set up the output pipe!\n");
                continue;
            }

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
        
            // Setting up the first child!
            int firstID = fork();

            // Handling the first command!
            if(firstID == 0) {
                // Closing the read end, since this child won't be reading anything from the shell pipe!
                close(pipeDescriptors[0]);
        
                // Changing stdout's file descriptor!
                dup2(pipeDescriptors[1], STDOUT_FILENO);
                dup2(pipeDescriptors[1], STDERR_FILENO);
                close(pipeDescriptors[1]);
        
                // Running the first command!
                int error = execvp(firstCommand[0], firstCommand);
                if(error == -1) {
                    // The child process must never print to the terminal, we want only the parent process to do so! This is because "ncurses" will work only with the parent process, and avoid mix-up of the window we created and the default terminal!
                    exit(1);
                }
            }

            // Setting up the second child!
            int secondID = fork();
            if(secondID == 0) {
                // Closing the write end, since this child won't be writing anything to the shell pipe!
                close(pipeDescriptors[1]);

                // Changing stdin's file descriptor!
                dup2(pipeDescriptors[0], STDIN_FILENO);
                close(pipeDescriptors[0]);

                // Re-directing the final output to the parent!
                close(outputPipe[0]);
                dup2(outputPipe[1], STDOUT_FILENO);
                dup2(outputPipe[1], STDERR_FILENO);
                close(outputPipe[1]);
            
                // Running the second command!
                int error = execvp(secondCommand[0], secondCommand);
                if(error == -1) {
                    exit(1);
                }
            }

            // Closing the shell pipe completely, since parent does not read/write to it!
            close(pipeDescriptors[0]);
            close(pipeDescriptors[1]);
            
            // Closing the write end of the output pipe, parent will only read from it!
            close(outputPipe[1]);
            
            // Parent will now display the output!
            displayOutput(outputPipe[0]);
            redrawScreen();
            close(outputPipe[0]);

            wait(NULL);
            wait(NULL);
            continue;
        }
        
        // Fixing normal commands!
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

        // Output pipe to send the output to the parent process, then the parent process will display it!
        int outputPipe[2];
        if(pipe(outputPipe) == -1) {
            strcat(terminalBuffer, "Error: Could not create output pipe!\n");
            continue;
        }

        // Creating the child process to run the execvp() command!
        int processID = fork();
        if(processID == -1) {
            strcat(terminalBuffer, "Error: Could not fork!\n");
            continue;
        }

        // Child process here!
        if(processID == 0) {
            // Child does not read from the output pipe!
            close(outputPipe[0]);
        
            // Re-directing output to the output pipe!
            dup2(outputPipe[1], STDOUT_FILENO);
            dup2(outputPipe[1], STDERR_FILENO);
            close(outputPipe[1]);
            
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

                // This checks if an error occurred while opening the file!
                if(currentFile == -1) {
                    exit(1);
                }

                dup2(currentFile, STDOUT_FILENO);
                dup2(currentFile, STDERR_FILENO);
                close(currentFile);
        
                // Running the execvp() command!
                int error = execvp(beforeAngular[0], beforeAngular);
                if(error == -1) {
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
                
                if(currentFile == -1) {
                    exit(1);
                }

                dup2(currentFile, STDIN_FILENO);
                close(currentFile);

                // Child will not be reading from the output pipe!
                close(outputPipe[0]);

                // Re-directing output to the output pipe!
                dup2(outputPipe[1], STDOUT_FILENO);
                dup2(outputPipe[1], STDERR_FILENO);
                close(outputPipe[1]);

                // Running the execvp() command!
                int error = execvp(beforeAngular[0], beforeAngular);
                if(error == -1) {
                    exit(1);
                }
            }
        
            // If no "<" or ">" was found, then it must be a normal command!
            else {
                execvp(portions[0], portions);
                exit(1);
            }
        }

        // Now the parent will handle displaying the output!
        else {
            // Parent does not read from the output pipe!
            close(outputPipe[1]);
        
            // Parent now display's the output!
            displayOutput(outputPipe[0]);
            redrawScreen();
            close(outputPipe[0]);
        
            // Waiting for child process to finish execution!
            wait(NULL);
            continue;
        }
    }
    endwin();
    return 0;
}