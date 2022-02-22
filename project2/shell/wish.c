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
    linkedlist argv;   // List of all tokens for this command
    linkedlist redirection;
} command;

static void RunInteractiveMode(void);
static void RunBatchMode(char *file_path);

/* GLOBAL VARIABLES */
const char error_message[30] = "An error has occurred\n";

/* CODE */
int main(int argc, char *argv[]) {
    if (argc == INTERACTIVE_MODE) {
        Run(INTERACTIVE_MODE, NULL);
    } else if (argc == BATCH_MODE) {
        Run(BATCH_MODE, argv[BATCH_MODE-1]);
    } else {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    return 0;
}

static void InitList(linkedlist *list) {
    assert(list);
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

/*
    Creates an empty list
    Must free once done with it
*/
static linkedlist *CreateList(void) {
    linkedlist *new_list = (linkedlist *) malloc(sizeof(linkedlist));
    assert(new_list);
    if (new_list != NULL) {
        InitList(new_list);
    }

    return new_list;
}

/* Inserts a new node at the end  */
static void InsertList(linkedlist *list, void *data) {
    assert(list);
    node *new_node = (node *) malloc(sizeof(node));
    assert(new_node);
    if (list->count == 0) {
        new_node->next = new_node;
        new_node->prev = new_node;
        list->head = new_node;
        list->tail = new_node;
    } else {
        new_node->data = data;
        new_node->next = list->head;
        new_node->prev = list->tail;
        list->head->prev = new_node;
        list->tail->next = new_node;
    }

    list->count++;
}

/* Returns pointer to the data of the node that removed */
static void *PopList(linkedlist *list) {
    assert(list);
    if (list->count > 0) {
        node *node_to_remove = list->head;
        void *data_to_return = node_to_remove->data;
        list->head = node_to_remove->next;
        list->tail->next = node_to_remove->next;
        free(node_to_remove);
        list->count--;
        if (list->count == 0) {
            list->head = NULL;
            list->tail = NULL;
        }
        return data_to_return;
    }

    return NULL;
}

static void ResetPath(linkedlist *path) {
    while(path->count > 0) {
        free(PopList(path));
    }
}

/* Initializes command by parsing input line and putting it in cmd_list.
   If len(linkedlist) > 1, then it is a parallel command
*/
static void ParseCommandList(linkedlist *cmd_list, char *line) {
    char *line_copy = strdup(line);
    linkedlist *cmd_list = (linkedlist *) malloc(sizeof(linkedlist));
    char *parallel_sep_ptr, *whitespace_sep_ptr, *redirection_sep_ptr;

    /* First separate based on '&' and newline */
    while ((parallel_sep_ptr = strsep(&line_copy, "&\n")) != NULL) {
        if (*parallel_sep_ptr != '\0') {
            command *new_cmd = (command *) malloc(sizeof(command));
            new_cmd->redirection = NULL;
            new_cmd->argv = NULL;
            new_cmd->argc = 0;
            //printf("p*%s*p\n", parallel_sep_ptr);
            /* Then parse based on redirection */
            while ((redirection_sep_ptr = strsep(&parallel_sep_ptr, ">")) != NULL) {
                if (*redirection_sep_ptr != '\0') {
                    //printf("r*%s*r\n", redirection_sep_ptr);
                    /* Finally parse on whitespaces */
                    while ((whitespace_sep_ptr = strsep(&redirection_sep_ptr, " \t")) != NULL) {
                        if (*whitespace_sep_ptr != '\0') {
                            //printf("w*%s*w\n", whitespace_sep_ptr);
                        }
                    }
                }
            }
        }
    }

    free(line_copy);
    return cmd_list;
}

/* User must call DeleteCommand once done with popped command */
static command *PopCommandList(linkedlist *cmd_list) {

}

static void DeleteCommand(command *cmd) {

}

/* Frees up malloced data and resets variables */
static void ResetCommandList(linkedlist *cmd_ptr) {

}


static void Run(int mode, char *file_path) {
    FILE *fp;
    char *line;
    size_t nread, len;
    bool is_parallel;
    linkedlist path, cmds;

    if (mode == BATCH_MODE) {
        /* Try to open the file. Throw error and quit if it cannot be opened */
        fp = fopen(file_path, "r");
        if (fp == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    } else {
        fp = stdin;
    }
    
    /* Initialize the path list */
    InitList(&path);
    InsertList(&path, strdup("/bin"));

    while(1) {
        if (mode == INTERACTIVE_MODE) {
            printf("wish> ");
        }

        if ((nread = getline(&line, &len, stdin)) != -1) {
            ParseCommandList(&cmds, line);
            is_parallel = (cmds.count > 1);
            if (!is_parallel) {
                /* Single command. Either built in command or system program*/
                command *current_cmd = PopCommandList(&cmds);
                if (current_cmd != NULL) {
                    char *first_arg = PopList(&current_cmd->argv);
                    if (first_arg != NULL) {
                        if (strcmp(first_arg, "exit") == 0) {
                            if (current_cmd->argv.count > 0) {
                                /* Error */
                                write(STDERR_FILENO, error_message, strlen(error_message));
                            } else {
                                DeleteCommand(current_cmd);
                                ResetCommandList(&cmds);
                                free(first_arg);
                                exit(0);
                            }
                        } else if (strcmp(first_arg, "cd") == 0) {
                            if (current_cmd->argv.count > 1) {
                                write(STDERR_FILENO, error_message, strlen(error_message));
                            } else {
                                char *cd_path = PopList(&current_cmd->argv);
                                if (chdir(cd_path) == -1) {
                                    /* chdir failed so throw an error */
                                    write(STDERR_FILENO, error_message, strlen(error_message));
                                }
                                free(cd_path);
                            }
                        } else if(strcmp(first_arg, "path") == 0) {
                            if (current_cmd->argv.count == 0) {
                                /* Reset path */
                                ResetPath(&path);
                            } else {
                                /* Iterate through all the arguments and add to path */
                                while(current_cmd->argv.count > 0){
                                    char *curr_loc = PopList(&current_cmd->argv);
                                    InsertList(&path, strdup(curr_loc));
                                    free(curr_loc);
                                }
                            }
                        } else {
                            /* System command. */
                            /* Iterate through all the paths and see if command is in there */
                        }

                        free(first_arg);
                    }

                    DeleteCommand(current_cmd);
                }
            } else {
                while(cmds.count > 0) {
                    command *current_cmd = PopCommandList(&cmds);
                }
                /* Wait for all of them to finish */
            }



            ResetCommandList(&cmds);
        } else {
            if (mode == BATCH_MODE) {
                /* EOF, exit gracefully */
                goto EXIT;
            }
        }
    }

EXIT:
    DeinitPath(&path);
    DeinitCommand(&cmds);
    exit(0);
}

#if 0
static void RunInteractiveMode(void) {
    size_t nread, len;
    char *line = NULL;
        printf("wish> ");
        if ((nread = getline(&line, &len, stdin)) != -1) {
            /* Parse the command and check if  it's a built in command */
            linkedlist *cmd_list = CreateCommand(line);
        }    

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
#endif