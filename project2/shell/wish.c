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
#include <stdbool.h>

/* DEFINES */
#define INTERACTIVE_MODE 1
#define BATCH_MODE       2

/* Only use for path but will use for command if need be */
typedef struct _node_ {
    void *data;
    struct _node_ *next;
    struct _node_ *prev;
} node;

typedef struct _linkedlist_ {
    node *head;
    node *tail;
    int count;
}linkedlist;

typedef struct {
    linkedlist *argv;   // List of all tokens for this command
    int argc;   // Number of tokens
    int is_parallel;    // If we are trying to execute parallel commands
    bool is_redirected;
    node *redirection;
} command;

/* FUNCTION PROTOTYPES */
// static void RunInteractiveMode(void);
// static void RunBatchMode(char *file_path);
// static node *CreatePath(void);
// static void AppendToPath(node **path, char *new_loc);
// static void DestroyPath(node **path);
// static command *CreateCommand(char *line);
// static void DestroyCommand(command **cmd);

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

    return 0;
}

/*
    Creates an empty list
    Must free once done with it
*/
static linkedlist *CreateList(void) {
    linkedlist *new_list = (linkedlist *) malloc(sizeof(linkedlist));
    if (new_list != NULL) {
        new_list->head = NULL;
        new_list->tail = NULL;
        new_list->count = 0;
    }

    return new_list;
}

/* Inserts a new node at the end  */
static void InsertList(linkedlist *list, void *data) {
    node *new_node = (node *) malloc(sizeof(node));
    new_node->data = data;
    new_node->next = list->head;
    new_node->prev = list->tail;
    list->head->prev = new_node;
    list->tail->next = new_node;
    list->count++;
}

/* Returns pointer to the data of the node that removed */
static void *PopList(linkedlist *list) {
    node *node_to_remove = list->head;
    void *data_to_return = node_to_remove->data;
    list->head = node_to_remove->next;
    list->tail->next = node_to_remove->next;
    free(node_to_remove);
    list->count--;
    return data_to_return;
}


/*
   Initital will always contain /bin
 */
static linkedlist *CreatePath(void) {
    linkedlist *path = CreateList();
    assert(path != NULL);

    /* insert node for bin */
    PushList(path, strdup("/bin"));

    return path;
}

static void AppendToPath(linkedlist *path, char *loc) {
    PushList(path, strdup(loc));
}

/* Deallocate everything */
static void DestroyPath(linkedlist *path) {
    while (path->count > 0) {
        char *popped_data = (char *) PopList(path);
        free(popped_data);
    }
    free(path);
}

/* Parses string into cmd + arguments based on space character
   Must call DestroyCommand when you no longer need it.
 */
static command *CreateCommand(char *line) {
    command *cmd = (command *) malloc(sizeof(command));
    assert(cmd != NULL);
    cmd->argc = 0;
    cmd->parallel_cnt = 0;

    char *tok = strtok(line, " \n\t");
    while(tok != NULL) {
        node *new_node = (node *) malloc(sizeof(node));
        if (cmd->argc == 0) {
            new_node->prev = new_node;
            new_node->next = new_node;
            cmd->argv = new_node;
        } else {
            cmd->argv->prev->next = new_node;
            new_node->prev = cmd->argv->prev;
            new_node->next = cmd->argv;
            cmd->argv->prev = new_node;
        }

        char *new_payload = (char *)malloc(strlen(tok) + 1);
        strcpy(new_payload, tok);
        if (strcmp(new_payload, "&") == 0) {
            cmd->parallel_cnt++;
        }
        new_payload[strlen(tok)] = 0;  //NULL terminate the new string
        new_node->payload = (void *) new_payload;
        cmd->argc++;
        tok = strtok(NULL, " \n\t");
    }

    if (cmd->parallel_cnt > 0) {
        /* Add 1 for the last parallel command */
        cmd->parallel_cnt++;
    }

    return cmd;
}

static void DestroyCommand(command **cmd_ptr) {
    command *cmd = *cmd_ptr;
    node *curr_arg = cmd->argv->next;
    while (curr_arg != cmd->argv) {
        node *temp = curr_arg->next;
        free(curr_arg->payload);
        free(curr_arg);
        curr_arg = temp;
        cmd->argc--;
    }

    free(cmd->argv->payload);
    free(cmd->argv);
    cmd->argc--;
    free(cmd);
    *cmd_ptr = NULL;
}

static void RunInteractiveMode(void) {
    size_t nread, len;
    char *line = NULL;

    node *path = CreatePath();

    while (1) {
        printf("wish> ");
        if ((nread = getline(&line, &len, stdin)) != -1) {
            /* Parse the command and check if  it's a built in command */
            command *cmd = CreateCommand(line);
            if (cmd->argc > 0) {
                if (strcmp(cmd->argv->payload, "exit") == 0) {
                    /* Throw error if any arguments are passed to exit */
                    if (cmd->argc > 1) {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    } else {
                        DestroyPath(&path);
                        DestroyCommand(&cmd);
                        free(line);
                        exit(0);
                    }
                } else if (strcmp(cmd->argv->payload, "cd") == 0) {
                    /* If 0 or more than 1 argument is passed to cd, then throw error */
                    if (cmd->argc != 2) {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    } else {
                        if (chdir(cmd->argv->next->payload) == -1) {
                            /* chdir failed so throw an error */
                            write(STDERR_FILENO, error_message, strlen(error_message));
                        }
                    }
                } else if (strcmp(cmd->argv->payload, "path") == 0) {
                    if (cmd->argc == 1) {
                        /* No arguments passed, so clear current path */
                        DestroyPath(&path);
                        path = NULL;
                    } else {
                        /* Iterate through all the arguments and add them to path */
                        node *curr_arg = cmd->argv->next;
                        while (curr_arg != cmd->argv) {
                            AppendToPath(&path, curr_arg->payload);
                            curr_arg = curr_arg->next;
                        }
                    }
                } else {
#if 1
                        node *curr = cmd->argv;
                        for (int i = 0; i < cmd->argc; i++) {
                            if (strcmp(curr->payload, "&") == 0) {
                                printf("\n-------\n");
                            } else {
                                printf("*%s*", (char *) curr->payload);
                            }
                            curr = curr->next;
                        }
#endif
#if 0
                    /* Code to test that parallel commands are being parsed correctly */
                    printf("Parallel Count = %d\n", cmd->parallel_cnt);
                    if (cmd->parallel_cnt > 0) {
                        node *curr = cmd->argv;
                        for (int i = 0; i < cmd->argc; i++) {
                            if (strcmp(curr->payload, "&") == 0) {
                                printf("\n-------\n");
                            } else {
                                printf("*%s*", (char *) curr->payload);
                            }
                            curr = curr->next;
                        }
                    }
#endif
                }
            }
            DestroyCommand(&cmd);
        }
    }
}

static void RunBatchMode(char *file_path) {
    //node *path = CreatePath();
    printf("Have not implemented this");
}