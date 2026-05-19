# Custom Linux Terminal in C

A custom Linux terminal built from scratch in C using:
- fork()
- execvp()
- wait()
- File descriptor manipulation
- dup2()
- ncurses

The project currently supports:
- Execution of some standard Linux commands
- Input re-direction ('<')
- Output re-direction ('>')
- Built-in commands like 'cd', 'clear' and 'exit'
- A full-screen 'ncurses' terminal interface

# Features
## 1. Standard Command Execution

Run normal Linux commands directly inside the custom terminal.

Examples:
- ls
- pwd
- echo

Implemented using:
- fork()
- execvp()
- wait()

## 2. Input Re-Direction

Re-direct a file into the command's standard input.

Example:
- cat < input.txt

The shell:
- Opens the file
- Re-directs STDIN_FILENO
- Executes the command

Implemented using:
- open()
- dup2()

## 3. Output Re-Direction

Re-direct command output into a file.

Example:
- ls > output.txt

The shell:
- Creates/truncates the file
- Re-directs STDOUT_FILENO
- Executes the command

Implemented using:
- open()
- dup2()

## 4. Built-In Commands

### 4.1. cd

Changes the current working directory.

Example:
- cd ..

### 4.2. clear

Clears the custom terminal window.

### 4.3. exit

Gracefully exits the shell.

## 5. Pipe Command

Pass the output of one command directly as the input to another command.

Example:
- ls | grep .c

The shell:
- Creates a pipe for the inter-process communication
- Forks two child processes
- Re-directs the first command's 'STDOUT_FILENO' to the pipe
- Re-directs the second command's 'STDIN_FILENO' to the pipe
- Executes the commands simultaneously

Implemented using:
- pipe()
- fork()
- dup2()
- execvp()

## 6. ncurses Interface

The shell runs inside a full-screen terminal interface built using ncurses.

Features:
- Full-screen UI
- Persistent terminal buffer
- Custom rendering
- Controlled cursor positioning
- Themed interface

## Technologies Used
- C
- Linux/POSIX APIs
- ncurses

# Compilation

This repository contains multiple development phases of the custom shell project:
- 01basicshell.c
- 02outputredirection.c
- 03inputredirection.c
- 04pipesupport.c
- 05integratedshell.c

Each file represents a specific stage in the shell's development:
- Standard command execution
- Input re-direction ('<')
- Output re-direction ('>')
- Pipe support ('|')
- Build-in commands

These files are preserved separately for learning/reference purposes.

# Final Integrated Build

The fully combined and working implementation is contained in:
- terminal.c

The project uses a Makefile for compilation. To compile the final shell, run the following commands:
- make
- ./terminix

Notes:
- The Makefile automatically handles linking against required libraries such as ncurses.
- Earlier phases (located in the 'phases' folder) are not intended to be compiled together.
- Only terminal.c represents the final integrated version of the shell.

# Concepts Learned

This project explores:
- Process creation with fork()
- Program execution using execvp()
- Process synchronization with wait()
- File descriptor manipulation
- Input/output re-direction
- Terminal UI programming with ncurses
- Shell architecture

# Current Limitations

Currently unsupported:
- Multiple pipes
- Command history
- Auto-complete
- Background processes
- Signal handling
- Custom ncurses interface
