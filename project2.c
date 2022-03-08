#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLEN 1000

int fds[100][2];

/* Function: count_pipes
 * Author: David Allen
 * Params:
 * -char* line, the full string
 * Returns: int number of pipes (|)
 * Purpose: count how many pipes need to be created
 */
int count_pipes(char* line) {
        char temp[MAXLEN];
        strcpy(temp, line);
        int i=0;

        for (char* cmd = strtok(temp, "|"); cmd && *cmd; cmd = strtok(NULL, "|") )
                i++;

        return i - 1;

}

/* Function: parse_command
 * Author: David Allen
 * Params:
 * -char* line, the full command string;
 * -char* arguments[], an string array to store arguments;
 * Returns: int number of args
 * Purpose: parses the command into arguments to be read by execvp()
 */
int parse_command(char *line, char *arguments[]) {

        char *command = strtok(line, " \n");
        arguments[0] = command;

        int i=1;

        while ((arguments[i] = strtok(NULL, " \n")) != NULL)
                i++;

        return i;
}


int main(int argc, char *argv[]) {
        while (1) {
                char cmdLine[MAXLEN] = "";
                char* cmd;

                printf("%d> ", getpid());

                // get commandline
                if (fgets(cmdLine, MAXLEN, stdin) == NULL) {
                        puts("Program terminated by user...");
                        return 0;
                }

                int numPipes = count_pipes(cmdLine);
                int i = 0;

                for (cmd = strtok(cmdLine, "|\n"); cmd && *cmd; cmd = strtok(NULL, "|\n"), i++) {
                        // create pipe
                        if (i < numPipes && pipe(fds[i]) == -1) {
                                fprintf(stderr, "pipe failed.\n");
                                return 1;
                        }

                        pid_t pid = fork();
                        if (pid == 0) {
                                char *args[MAXLEN];
                                parse_command(cmd, args);

                                if (i < numPipes) {
                                        // redirect stdout to write end of pipe
                                        if (dup2(fds[i][1], 1) < 0 ){
                                                puts("can't dup pipe write");
                                        }
                                        close(fds[i][1]); // close 2
                                        close(fds[i][0]); // close 3

                                }
                                // dup2 redirects read end of pipe to stdin
                                if (i > 0) {
                                        //close(0);
                                        dup2(fds[i-1][0], 0);
                                        close(fds[i-1][0]); // close 1
                                        close(fds[i-1][1]); // close 0
                                }

                                //Executing cmd;
                                if (execvp(args[0], args) < 0) {
                                        fprintf(stderr, "error with command, exec failed.\n");
                                        return 1;
                                }
                                return 2;
                        }
                        else if (pid < 0) {
                                // error check
                                fprintf(stderr, "fork failed.\n");
                                close(fds[i][0]);
                                close(fds[i][1]);
                                return 1;
                        }
                        close(fds[i][1]);
                        if (i > 0) {
                                close(fds[i-1][0]);
                                close(fds[i-1][1]);
                        }
                        wait(NULL);
                }
                wait(NULL);
        }

}
