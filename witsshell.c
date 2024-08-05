#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

// Function to execute a command
void execute_command(char *line) {
    char *args[100]; // Array to hold command arguments
    int i = 0;

    // Tokenize the input line into arguments
    args[i] = strtok(line, " ");
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " ");
    }

    // If no command is entered, return
    if (args[0] == NULL) {
        return;
    }

    // If the command is "exit", terminate the shell
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    // Create a new process to execute the command
    pid_t pid = fork();
    if (pid == 0) {
        // In the child process, execute the command
        execvp(args[0], args);
        // If execvp fails, print an error message and exit
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // In the parent process, wait for the child to finish
        wait(NULL);
    } else {
        // If fork fails, print an error message
        perror("fork");
    }
}

int main(int argc, char *argv[]) {
    char *line = NULL; // Pointer to hold the input line
    size_t len = 0; // Length of the input line
    ssize_t nread; // Number of characters read
    FILE *input = stdin; // Input source (default is stdin)

    // If a file is provided as an argument, open it for reading
    if (argc > 1) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
    }

    // Main loop to read and execute commands
    while (true) {
        // If reading from stdin, print the shell prompt
        if (input == stdin) {
            printf("witsshell> ");
        }

        // Read a line of input
        nread = getline(&line, &len, input);
        if (nread == -1) {
            // If end of file is reached, break the loop
            if (feof(input)) {
                break;
            } else {
                // If an error occurs, print an error message and break the loop
                perror("getline");
                break;
            }
        }

        // Remove newline character from the end of the line
        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        // Execute the command
        execute_command(line);
    }

    // Free the allocated memory for the input line
    free(line);
    // If reading from a file, close the file
    if (input != stdin) {
        fclose(input);
    }
    return 0;
}