#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

/* DEFINES */
#define INTERACTIVE_MODE 1
#define BATCH_MODE       2

/* Only use for path but will use for command if need be */
typedef struct _node_ {
    void *payload;
    struct _node_ *next;
    struct _node_ *prev;
} node;

typedef struct {
    char **argv;
    int argc;
} command;

/* FUNCTION PROTOTYPES */
static void RunInteractiveMode(void);
static void RunBatchMode(char *file_path);
static node *CreatePath(void);
static void AppendToPath(node **path, char *new_loc);
static void DestroyPath(node **path);
static command *CreateCommand(char *line);
static void DestroyCommand(command **cmd);

/* GLOBAL VARIABLES */
const char error_message[30] = "An error has occurred\n";

/* CODE */
int main(int argc, char *argv[]) {
    if (argc == INTERACTIVE_MODE) {
        RunInteractiveMode();
    } else if (argc == BATCH_MODE) {
        RunBatchMode(argv[BATCH_MODE-1]);
    } else {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    exit(0);
}

/*
   Initital will always contain /bin
 */
static node *CreatePath(void) {
    node *path = (node *) malloc(sizeof(node));
    assert(path != NULL);

    /* Create string for new path location */
    int bin_buff_size = strlen("/bin") + 1;
    char *bin_buff = (char *) malloc(bin_buff_size);
    strcpy(bin_buff, "/bin");
    bin_buff[bin_buff_size - 1] = 0;

    path->payload = (void *) bin_buff;
    path->next = path;
    path->prev = path;

    return path;
}

/* Deallocate everything */
static void DestroyPath(node **path) {
    node *head = *path;
    node *curr_node = head->next;
    while(curr_node != head) {
        node *temp = curr_node->next;
        free(curr_node->payload);
        free(curr_node);
        curr_node = temp;
    }

    free(head->payload);
    free(head);
    *path = NULL;
}

static void AppendToPath(node **path, char *new_loc) {
    node *new_node = (node *) malloc(sizeof(node));
    assert(path != NULL);

    /* Create string for new path location */
    int new_loc_buff_size = strlen(new_loc) + 1;
    char *new_loc_buff = (char *) malloc(new_loc_buff_size);
    strcpy(new_loc_buff, new_loc);
    new_loc_buff[new_loc_buff_size - 1] = 0;
    new_node->payload = (void *) new_loc_buff;

    node *head = *path;
    if (head != NULL) {
        /* Add new node and stitch things up */
        head->prev->next = new_node;
        new_node->prev = head->prev;
        new_node->next = head;
        head->prev = new_node;
    } else {
        new_node->next = new_node;
        new_node->prev = new_node;
        *path = new_node;
    }
}

/* Parses string into cmd + arguments based on space character
   Must call DestroyCommand when you no longer need it.
 */
static command *CreateCommand(char *line) {
    command *cmd = (command *) malloc(sizeof(command));
    assert(cmd != NULL);

    /* Count number of total tokens first */
    char *temp_ptr;
    int arg_cnt = 1;
    while((temp_ptr = strpbrk(temp_ptr, " ")) != NULL) {
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

    return cmd;
}

static void DestroyCommand(command **cmd_ptr) {
    command *cmd = *cmd_ptr;
    while (cmd->argc > 0) {
        free(cmd->argv[cmd->argc]);
        cmd->argc--;
    }

    free(cmd->argv);
    free(cmd);
    *cmd_ptr = NULL;
}

static void RunInteractiveMode(void) {
    size_t nread, len;
    char *line;

    node *path = CreatePath();

    while (1) {
        printf("wish> ");
        if ((nread = getline(&line, &len, stdin)) != -1) {
            /* Parse the command and check if  it's a built in command */
            command *cmd = CreateCommand(line);
            if (cmd->argc > 0) {
                if (strcmp(cmd->argv[0], "exit") == 0) {
                    /* Throw error if any arguments are passed to exit */
                    if (cmd->argc > 1) {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    } else {
                        DestroyPath(&path);
                        DestroyCommand(&cmd);
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
                        DestroyPath(&path);
                        path = NULL;
                    } else {
                        /* Iterate through all the arguments and add them to path */
                        for (int i = 1; i < cmd->argc; i++) {
                            AppendToPath(&path, cmd->argv[i]);
                        }
                    }
                } else {
                    printf("still haven't implemented this part.");
                }
            }
            DestroyCommand(&cmd);
        }
        free(line);
    }
}

static void RunBatchMode(char *file_path) {
    //node *path = CreatePath();
    printf("Have not implemented this");
}