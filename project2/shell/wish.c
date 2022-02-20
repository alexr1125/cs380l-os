#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* DEFINES */
#define INTERACTIVE_MODE 2
#define BATCH_MODE       3

/* FUNCTION PROTOTYPES */
static void InteractiveModeRun(void);
static void BatchModeRun(char *file_path);
static char** CreatePath(void);
static void AppendToPath(char** path, char *new_loc);
static void EmptyPath(char **path);
static void DestroyPath(char **path);
static command *CreateCommand(char *line);
static void DestroyCommand(command *cmd);
/* GLOBAL VARIABLES */
const char error_message[30] = "An error has occurred\n";

typedef struct {
    char **argv;
    int argc;
} command;

/* CODE */
int main(int argc, char *argv[]) {
    if (argc == INTERACTIVE_MODE) {
        InteractiveModeRun();
    } else if (argc == BATCH_MODE) {
        BatchModeRun(argv[BATCH_MODE-1]);
    } else {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
}

/* Last element will always be NULL pointer */
static char** CreatePath(void) {

}

/* Deallocate everything */
static void DestroyPath(char **path) {

}

static void AppendToPath(char** path, char *new_loc) {

}

/* Removes all locations in path */
static void EmptyPath(char **path) {

}
/* Parses string into cmd + arguments based on space character
   Must call DestroyCommand when you no longer need it.
 */
static command *CreateCommand(char *line) {
    command *cmd = (command *)malloc(sizeof(command));

    /* Count number of total tokens first */
    char *temp_ptr;
    int arg_cnt = 1;
    while((temp_ptr = strpbrk(temp_ptr, " \t")) != NULL) {
        arg_cnt++;
    }

    /* Allocate space for argument array */
    *cmd->argv = (char *) malloc(sizeof(char *) * (arg_cnt + 1));
    cmd->argv[arg_cnt] = NULL;

    char *tok;
    int i = 0;
    while((tok = strtok(line, " ")) != NULL) {
        cmd->argv[i] = (char *)malloc(strlen(tok) + 1);
        strcpy(cmd->argv[i], tok);
        cmd->argv[i][strlen(tok)] = 0;  //NULL terminate the new string
        i++;
        cmd->argc++;
    }

    assert(arg_cnt == cmd->argc);
}

static void DestroyCommand(command *cmd) {
    while (cmd->argc > 0) {
        free(cmd->argv[cmd->argc]);
        cmd->argc--;
    }

    free(cmd->argv);
    free(cmd);
}

static void RunInteractiveMode(void) {
    size_t nread, len;
    char *line;
    char *current_token;

    char **path = CreatePath();

    while (1) {
        printf("wish> ");
        if ((nread = getline(&line, &len, stdin)) != -1) {
            /* Parse the command and check if  it's a built in command */
            command *cmd = ParseCommand(line);
            if (cmd->argc > 0) {
                if (strcmp(cmd->argv[0], "exit") == 0) {
                    /* Throw error if any arguments are passed to exit */
                    if (cmd->argc > 1) {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    } else {
                        DestroyPath(path);
                        DestroyCommand(cmd);
                        exit(0);
                    }
                } else if (strcmp(cmd->argv[0], "cd") == 0) {
                    /* If 0 or more than 1 argument is passed to cd, then throw error */
                    if (cmd->argc != 2) {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    } else {
                        if (chdir(cmd->argv[1]) == -1) {
                            /* chdir failed so throw an error */
                            write(STDERR_FILENO, error_message, strlen(error_message));
                        }
                    }
                } else if (strcmp(cmd->argv[0], "path") == 0) {
                    if (cmd->argc == 1) {
                        EmptyPath(path);
                    } else {
                        /* Iterate through all the arguments and add them to path */
                        for (int i = 1; i < cmd->argc; i++) {
                            AppendToPath(path, cmd->argv[i]);
                        }
                    }
                } else {
                    printf("still haven't implemented this part.");
                }
            }
            DestroyCommand(cmd);
        }
        free(line);
    }
}

static void RunBatchMode(char *file_path) {
    char **path = InitPath();
}