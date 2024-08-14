#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>


// all: witsshell

// witsshell: witsshell.c
// 	gcc witsshell.c -Wall -Wextra -pedantic -o witsshell
// clean:
// 	rm -f witsshell
char error_message[30] = "An error has occurred\n";
char *PATH_LIST[100] = { "/bin", NULL }; // Initialize PATH_LIST with /bin

// Function to print the error message
void print_error() {
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// Function to execute the `ls` command
// void execute_ls_command() {
//     struct dirent **namelist;
//     int n;

//     n = scandir(".", &namelist, NULL, alphasort);
//     if (n < 0) {
//         print_error();
//     } else {
//         for (int i = 0; i < n; i++) {
//             if (namelist[i]->d_name[0] != '.') {
//                 printf("%s\n", namelist[i]->d_name);
//             }
//             free(namelist[i]);
//         }
//         free(namelist);
//     }
// }

// Function to execute a single command
void execute_single_command(char *line, char **path) {
    char *args[100]; // Array to hold command arguments
    int i = 0;
    char *filename = NULL;
    bool redirect = false;
    bool background = false;

    // Tokenize the input line into arguments using strsep
    while ((args[i] = strsep(&line, " ")) != NULL) {
        if (strlen(args[i]) > 0) {
            if (strcmp(args[i], ">") == 0) {
                redirect = true;
                args[i] = NULL; // Null-terminate the array before the redirection symbol
                filename = strsep(&line, " ");
                break;
            } else if (strcmp(args[i], "&") == 0) {
                background = true;
                args[i] = NULL; // Null-terminate the array before the background symbol
                break;
            }
            i++;
        }
    }
    args[i] = NULL; // Null-terminate the array

    // If no command is entered, return
    if (args[0] == NULL) {
        return;
    }

    // If the command is "exit", terminate the shell
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    // If the command is "cd", change the current working directory
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL || args[2] != NULL) {
            // If no directory is specified or more than one argument is provided, print an error message
            print_error();
        } else {
            // Change the directory using chdir
            if (chdir(args[1]) != 0) {
                // If chdir fails, print an error message
                print_error();
            }
        }
        return;
    }

    // If the command is "path", update the search path
    if (strcmp(args[0], "path") == 0) {
        // Clear the PATH_LIST if no arguments are provided
        if (args[1] == NULL) {
            path[0] = NULL;
        } else {
            // Update the search path with the directories specified in the arguments
            for (int j = 1; j < i; j++) {
                path[j - 1] = args[j];
            }
            path[i - 1] = NULL; // Null-terminate the array
        }
        return;
    }

    // Search for the executable in the directories specified by path
    char executable[1024];
    char *temp;
    for (i = 0; path[i] != NULL; i++) {
        snprintf(executable, sizeof(executable), "%s/%s", path[i], args[0]);
        if (access(executable, X_OK) == 0) {
            temp = executable;
            break;
        }
    }

    // If the executable is not found, print an error message and return
    if (path[i] == NULL) {
        print_error();
        return;
    }

    // Create a new process to execute the command
    pid_t pid = fork();
    if (pid == 0) {
        // In the child process, handle redirection if needed
        if (redirect && filename != NULL) {
            int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                print_error();
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        // Execute the command
        execv(temp, args);
        // If execv fails, print an error message and exit
        print_error();
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // In the parent process, wait for the child to finish if not running in the background
        if (!background) {
            waitpid(pid, NULL, 0);
        }
    } else {
        // If fork fails, print an error message
        print_error();
    }
}

// Function to execute multiple commands in parallel
void execute_parallel_commands(char *line, char **path) {
    char *commands[100]; // Array to hold individual commands
    int i = 0;

    // Tokenize the input line into individual commands using strsep
    while ((commands[i] = strsep(&line, "&")) != NULL) {
        if (strlen(commands[i]) > 0) {
            i++;
        }
    }
    commands[i] = NULL; // Null-terminate the array

    // Array to hold process IDs of child processes
    pid_t pids[100];
    int num_commands = i;

    // Execute each command in a separate child process
    for (i = 0; i < num_commands; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // In the child process, execute the command
            execute_single_command(commands[i], path);
            exit(EXIT_SUCCESS);
        } else if (pid > 0) {
            // In the parent process, store the child's PID
            pids[i] = pid;
        } else {
            // If fork fails, print an error message
            print_error();
        }
    }

    // Wait for all child processes to complete
    for (i = 0; i < num_commands; i++) {
        // Check if the command is meant to run in the background
        if (strstr(commands[i], "&") == NULL) {
            waitpid(pids[i], NULL, 0);
        }
    }
}

int main(int argc, char *argv[]) {
    char *line = NULL; // Pointer to hold the input line
    size_t len = 0; // Length of the input line
    ssize_t nread; // Number of characters read
    FILE *input = stdin; // Input source (default is stdin)

    // Initialize the search path with /bin/
    char *path[100] = { "/bin", NULL };

    // If more than one file is provided as an argument, print an error message and exit
    if (argc > 2) {
        print_error();
        exit(1);
    }

    // If a file is provided as an argument, open it for reading
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            print_error();
            exit(1);
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
                print_error();
                break;
            }
        }

        // Remove newline character from the end of the line
        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        // Check if the line contains parallel commands
        if (strstr(line, "&") != NULL) {
            // Execute parallel commands
            execute_parallel_commands(line, path);
        } else {
            // Execute a single command
            execute_single_command(line, path);
        }
    }

    // Free the allocated memory for the input line
    free(line);
    // If reading from a file, close the file
    if (input != stdin) {
        fclose(input);
    }
    return 0;
}