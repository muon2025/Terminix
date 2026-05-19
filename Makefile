terminix: terminal.c
	$(CC) terminal.c -lncurses -o terminix -Wall -Wextra -pedantic -std=c23
# We're telling the program to run the C-compiler, followed by the file name, the flag, the linker and the name for the executable!
# We've then put the -Wall flag, this instructs the compiler to display all warnings encountered when compiling the program. -Wextra and -pedantic are additional flags to capture more warnings!
# Finally, -std=c23 tells the compiler that we are using the C23 version!