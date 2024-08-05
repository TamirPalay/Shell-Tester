#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

int main(int MainArgc, char *MainArgv[]) {
    (void)MainArgc; // Mark MainArgc as unused
    (void)MainArgv; // Mark MainArgv as unused

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    while (true) {
        printf("witsshell> ");
        nread = getline(&line, &len, stdin);
        if (nread == -1) {
            perror("getline");
            break;
        }

        // Remove newline character from the end of the line
        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        if (strcmp(line, "exit") == 0) {
            break;
        }

        // Process the input line (placeholder for future commands)
    }

    free(line);
    return 0;
}