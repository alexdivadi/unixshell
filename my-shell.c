#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXLEN 1000

int fds[100][2];
int top = -1;
char* stack[MAXLEN];

/* Function: count_pipes
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


/* Function: checkSpecial
 * Params:
 * -char* line, the full string
 * Returns: int success (returns -1 if fail)
 * Purpose: Same function from assignment 1 to check for "exit", "quit", and "cd"
 */
int checkSpecial(char* lineIn[]) {
    if (strcmp(lineIn[0], "cd") == 0 && lineIn[1] != NULL) {
        if (chdir(lineIn[1]) != 0) {
            perror("Error changing directory");
        }
        
        return 1;
    }
    //check for pushd
    else if (strcmp(lineIn[0], "pushd") == 0 && lineIn[1] != NULL) {

        char path[MAXLEN];

        getcwd(path, MAXLEN);
        
        printf("Pushed: %s\n", path);
        
        top++;

        if(top >= MAXLEN) {
            perror("Stack full");
        }

        stack[top] = (char*) malloc(MAXLEN*sizeof(char));
        strcpy(stack[top], path);

        if (chdir(lineIn[1]) != 0) {
            perror("Error changing directory");
        }
        
        return 1;
    }
    //check for popd
    else if (strcmp(lineIn[0], "popd") == 0) {
        if(top < 0) {
            printf("Stack empty\n");
            return 1;
        }
        
        printf("Popped: %s\n", stack[top]);

        if (chdir(stack[top]) != 0) {
            perror("Error changing directory");
        }
        
        free(stack[top]);
        
        top--;
        
        return 1;
    }
    //check for dirs
    else if (strcmp(lineIn[0], "dirs") == 0) {
        for(int i = 0; i <= top; i++) {
          printf("[%d] %s\n", i, stack[i]);
        }
        
        return 1;
    }
    //check for exit and quit
    else if (strcmp(lineIn[0], "exit") == 0 || strcmp(lineIn[0], "quit") == 0) {
        exit(0);
    }
    
    return -1;
    //since command is not special, execute it as a system call, so return to main function
}

int main(int argc, char *argv[]) {
        while (1) {
                char cmdLine[MAXLEN] = "";

                printf("%d> ", getpid());

                // get commandline
                if (fgets(cmdLine, MAXLEN, stdin) == NULL) {
                        puts("Program terminated by user...");
                        return 0;
                }

                // Creates a copy of cmdLine to parse without affecting cmdLine, since cmdLine must get parsed later
                char copyLine[MAXLEN] = "";
                strcpy(copyLine, cmdLine);

                // Create array of all space-separated tokens, including potentially filename at beginning or end, and including pipes and <, >, >>
                char* tokens[MAXLEN];
                int numTokens = parse_command(copyLine, tokens);
                if(checkSpecial(tokens) == 1) {
                    continue;
                }
                
                // Check for < (input redirect)
                /*************************************************************/
                bool containsInRedirect = false;

                int hitPipe = 0;
                
                int num = 0;
                while (hitPipe == 0 && tokens[num] != NULL) { //stops when it finds a "|" or reaches end of line
                    if (strcmp(tokens[num], "|") == 0) {
                        hitPipe = num;
                    }
                    num++;
                }
                // if there is a pipe after first command, and first command contains at least 3 tokens (minimum for input redirect), then check back 2 tokens for a "<"
                if (hitPipe >= 3 && strcmp(tokens[hitPipe - 2], "<") == 0) {
                    containsInRedirect = true;
                }
                // if there is only one command and it contains input redirect
                else if (num >= 2 && strcmp(tokens[num - 2], "<") == 0) {
                    containsInRedirect = true;
                }
                
                // Check for > (output redirect) and >> (output append)
                /*************************************************************/
                bool containsOutRedirect = false;
                bool containsOutAppend = false;
                // There must be minimum 3 tokens in order for a command to redirect output to a file, and the second-to-last token must be ">"
                if (numTokens >= 3 && strcmp(tokens[numTokens - 2], ">") == 0) {
                    containsOutRedirect = true;
                }
                
                else if (numTokens >= 3 && strcmp(tokens[numTokens - 2], ">>") == 0) {
                    containsOutAppend = true;
                }
                

                int numPipes = count_pipes(cmdLine);
                int i = 0;
                
                for (char* cmd = strtok(cmdLine, "|\n"); cmd && *cmd; cmd = strtok(NULL, "|\n"), i++) {
                        // create pipe
                        if (i < numPipes && pipe(fds[i]) == -1) {
                                fprintf(stderr, "pipe failed.\n");
                                return 1;
                        }

                        pid_t pid = fork();

                        if (pid == 0) {
                                char *args[MAXLEN];
                                int numArgs = parse_command(cmd, args);

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
                                        if (dup2(fds[i-1][0], 0) < 0){
                                                puts("can't dup pipe read");
                                        }
                                        close(fds[i-1][0]); // close 1
                                        close(fds[i-1][1]); // close 0
                                }
                                if (i == 0 && containsInRedirect) {
                                    int fdin = open(args[numArgs - 1], O_RDWR);
                                    if (dup2(fdin, 0) < 0) {
                                        puts("Can't dup pipe read");
                                    }
                                    close(fdin);
                                }
                                if (i == numPipes && containsOutRedirect) {
                                    int fdout = open(args[numArgs - 1], O_RDWR | O_CREAT | O_TRUNC, 0777);
                                    if (dup2(fdout, 1) < 0) {
                                        puts("Can't dup pipe write");
                                    }
                                    close(fdout);
                                }
                                if (i == numPipes && containsOutAppend) {
                                    int fdout = open(args[numArgs - 1], O_RDWR | O_CREAT | O_APPEND, 0777);
                                    if (dup2(fdout, 1) < 0) {
                                        puts("Can't dup pipe write");
                                    }
                                    close(fdout);
                                }
                                //Executing cmd;
                                // If i/o file redirection is involved in current command
                                if ((i == 0 && containsInRedirect) || (i == numPipes && (containsOutRedirect || containsOutAppend))) {
                                    char* justArgs[numArgs - 1];
                                    for (int index = 0; index < numArgs - 2; index++) { // Exclude last two elements (e.g. "> filename")
                                        justArgs[index] = args[index];
                                    }
                                    justArgs[numArgs - 2] = NULL; // New array must contain NULL element after all arguments for execvp to work
                                    if (execvp(justArgs[0], justArgs) < 0) {
                                        fprintf(stderr, "error with I/O file redirection command, exec failed.\n");
                                        return 1;
                                    }
                                }
                                
                                else if (execvp(args[0], args) < 0) {
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
                        if (i < numPipes){
                                close(fds[i][1]);
                        }
                        if (i > 0) {
                                close(fds[i-1][0]);
                        }

                        wait(NULL);
                }
                wait(NULL);
                
        }

}
