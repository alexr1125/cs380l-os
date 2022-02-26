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
#include <fcntl.h>
#include <sys/wait.h>

/* DEFINES */
#define INTERACTIVE_MODE 1
#define BATCH_MODE       2

typedef struct _node_ {
    void *data;
    struct _node_ *next;
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

static void InitList(linkedlist *list);
static void InsertList(linkedlist *list, void *data);
static void *PopList(linkedlist *list);
static void ResetPath(linkedlist *path);
static void ParseCommandList(linkedlist *cmd_list, char *line);
static void DeleteCommand(command *cmd);
static void ResetCommandList(linkedlist *cmd_list_ptr);
static void Run(int mode, char *file_path);

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

/* Inserts a new node at the end  */
static void InsertList(linkedlist *list, void *user_data) {
    assert(list);
    node *new_node = (node *) malloc(sizeof(node));
    assert(new_node);
    new_node->data = user_data;
    new_node->next = NULL;
    if (list->count > 0) {
        list->tail->next = new_node;
        list->tail = new_node;
    } else {
        list->head = new_node;
        list->tail = new_node;
    }

    list->count++;
}

/* Returns pointer to the data of the node that was removed */
static void *PopList(linkedlist *list) {
    assert(list);

    void *user_data = NULL;
    if (list->head != NULL) {
        user_data = list->head->data;
        list->head = list->head->next;
        list->count--;
    }

    if (list->count == 0) {
        list->head = NULL;
        list->tail = NULL;
    }
    
    return user_data;
}

static void ResetPath(linkedlist *path) {
    if (path != NULL) {
        char *data_to_free;
        while((data_to_free = PopList(path)) != NULL) {
            free(data_to_free);
        }
    }
}

/* Initializes command by parsing input line and putting it in cmd_list.
   If len(linkedlist) > 1, then it is a parallel command.
   Error check here.
*/

static void ParseCommandList(linkedlist *cmd_list, char *line) {
    char *line_copy = line;
    char *parallel_sep_ptr, *whitespace_sep_ptr, *redirection_sep_ptr;
    command *new_cmd;

    /* First separate based on '&' and newline */
    while ((parallel_sep_ptr = strsep(&line_copy, "&\n")) != NULL) {
        if (*parallel_sep_ptr != '\0') {
            new_cmd = (command *) malloc(sizeof(command));
            InitList(&new_cmd->argv);
            InitList(&new_cmd->redirection);
            /* Then parse based on redirection if any */
            int redirection_i = 0;
            if (strstr(parallel_sep_ptr, ">") != NULL) {
                while ((redirection_sep_ptr = strsep(&parallel_sep_ptr, ">")) != NULL) {
                    if (*redirection_sep_ptr != '\0') {
                        /* Finally parse on whitespaces */
                        while ((whitespace_sep_ptr = strsep(&redirection_sep_ptr, " \t")) != NULL) {
                            if (*whitespace_sep_ptr != '\0') {
                                char *temp_ptr = strdup(whitespace_sep_ptr);
                                if (temp_ptr == NULL) {
                                    while(1);
                                }
                                if (redirection_i == 0) {
                                    InsertList(&new_cmd->argv, temp_ptr);
                                } else {
                                    InsertList(&new_cmd->redirection, temp_ptr);
                                }
                            }
                        }
                        redirection_i++;
                    }
                }

                /* Check if redirect is > 1. If so, then deallocate everything and return NULL */
                if ((redirection_i <= 1) || new_cmd->redirection.count > 1) {
                    /* Either too many redirects or missing redirect file */
                    DeleteCommand(new_cmd);
                    ResetCommandList(cmd_list);
                    /* Error occurred */
                    write(STDERR_FILENO, error_message, strlen(error_message));
                   break;
                }
            } else {
                /* parse on whitespaces since there is no redirection */
                while ((whitespace_sep_ptr = strsep(&parallel_sep_ptr, " \t")) != NULL) {
                    if (*whitespace_sep_ptr != '\0') {
                        char *temp_ptr = strdup(whitespace_sep_ptr);
                        if (temp_ptr == NULL) {
                            while(1);
                        }
                        InsertList(&new_cmd->argv, temp_ptr);
                    }
                }
            }
            InsertList(cmd_list, new_cmd);
        }
    }
}

static void DeleteCommand(command *cmd) {
    char *arg_str = PopList(&cmd->argv);
    while (arg_str != NULL) {
        free(arg_str);
        arg_str = PopList(&cmd->argv);
    }

    char *redirect_str = PopList(&cmd->redirection);
    while (redirect_str != NULL) {
        free(redirect_str);
        redirect_str = PopList(&cmd->redirection);
    }

    free(cmd);
}

/* Frees up malloced data and resets variables */
static void ResetCommandList(linkedlist *cmd_list_ptr) {
    assert(cmd_list_ptr);
    command *cmd = PopList(cmd_list_ptr);
    while(cmd!= NULL) {
        DeleteCommand(cmd);
        cmd = PopList(cmd_list_ptr);
    }
    
}


static void Run(int mode, char *file_path) {
#if 0
    linkedlist cmds,path;
    InitList(&path);
    InsertList(&path, strdup("/bin"));
    InitList(&cmds);
    FILE *fp;
    char *line;
    size_t nread, len;

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
    line = NULL;
    if ((nread = getline(&line, &len, fp)) != -1) {
        printf("line: *%s*\n", line);
        ParseCommandList(&cmds, line);
        command *cmd;
        int i = 0;
        while ((cmd = PopList(&cmds)) != NULL) {
            char *token;
            printf("CMD %d:\n", i);
            while ((token = PopList(&cmd->argv)) != NULL) {
                printf("*%s* ", token);
                free(token);
            }
            printf("\n------\n");
            while ((token = PopList(&cmd->redirection)) != NULL) {
                printf("*%s* ", token);
                free(token);
            }
            printf("\n------\n");    
            DeleteCommand(cmd);
            i++;
        }
        ResetCommandList(&cmds);
        free(line);
        line = NULL;
    }
    ResetPath(&path);
    return;
#else
    FILE *fp;
    char *line;
    size_t nread, len;
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
    char *initial_path = strdup("/bin");
    if (initial_path == NULL) {
        while(1);
    }
    InsertList(&path, initial_path);

    while(1) {
        if (mode == INTERACTIVE_MODE) {
            printf("wish> ");
        }
        line = NULL;
        if ((nread = getline(&line, &len, fp)) != -1) {
            InitList(&cmds);
            ParseCommandList(&cmds, line);
            free(line);
            command *current_cmd = PopList(&cmds);
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
                            ResetPath(&path);
                            exit(0);
                        }
                        free(first_arg);
                        DeleteCommand(current_cmd);                        
                    } else if (strcmp(first_arg, "cd") == 0) {
                        if (current_cmd->argv.count != 1) {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                        } else {
                            char *cd_path = PopList(&current_cmd->argv);
                            if (chdir(cd_path) == -1) {
                                /* chdir failed so throw an error */
                                write(STDERR_FILENO, error_message, strlen(error_message));
                            }
                            free(cd_path);
                        }
                        free(first_arg);
                        DeleteCommand(current_cmd);
                    } else if(strcmp(first_arg, "path") == 0) {
                        /* Clear path then iterate through all the arguments and add to path */
                        ResetPath(&path);
                        char *curr_loc;
                        while((curr_loc = PopList(&current_cmd->argv)) != NULL){
                            char *temp_ptr = strdup(curr_loc);
                            if (temp_ptr == NULL) {
                                while(1);
                            }
                            InsertList(&path, temp_ptr);
                            free(curr_loc);
                        }
                        free(first_arg);
                        DeleteCommand(current_cmd);
                    } else {
                        /* System command. */
                        while(current_cmd != NULL) {
                            /* Iterate through paths to see if cmd is in one of them */
                            node *current_path_node = path.head;
                            char *cmd_path = NULL;
                            while (current_path_node != NULL) {
                                int temp_path_str_len = strlen(current_path_node->data) + strlen(first_arg) + 1;
                                char *temp_path_str = (char *) malloc(temp_path_str_len + 1);
                                sprintf(temp_path_str, "%s/%s", (char *) current_path_node->data, first_arg);
                                if (access(temp_path_str, X_OK) == 0) {
                                    cmd_path = temp_path_str;
                                    break;
                                }
                                free(temp_path_str);
                                current_path_node = current_path_node->next;
                            }
                            
                            /* If command found in path, then get args and execute */
                            if (cmd_path != NULL){
                                int my_argc = current_cmd->argv.count + 1;  //+1 for the one that has already been popped
                                char **my_args = (char **)malloc(sizeof(char *)*(my_argc + 1)); //+1 for null termination
                                my_args[0] = first_arg;
                                my_args[my_argc] = NULL;    // NULL terminate the arg list
                                for (int i = 1; i < my_argc; i++) {
                                    my_args[i] = PopList(&current_cmd->argv);
                                }

                                char *redirect_file = NULL;
                                if(current_cmd->redirection.count > 0) {
                                    /* Redirect output to specified file */
                                    redirect_file = PopList(&current_cmd->redirection);
                                }

                                /* Create the child process and execute the command */
                                int rc = fork();
                                if (rc < 0) {
                                    /* Error */
                                } else if (rc == 0) {
                                    if (redirect_file != NULL) {
                                        close(STDOUT_FILENO);
                                        open(redirect_file, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);                                 
                                    }

                                    execv(cmd_path, my_args);
                                    _exit(0);
                                } else {
                                    /* Parent enters here */
                                    /* Free the memory allocated for the arg list */
                                    for (int i = 0; i < my_argc; i++) {
                                        free(my_args[i]);
                                    }
                                    free(my_args);
                                    free(cmd_path);

                                    if (redirect_file != NULL) {
                                        free(redirect_file);
                                    }
                                }
                            } else {
                                write(STDERR_FILENO, error_message, strlen(error_message));
                            }

                            /* Free current command memory and get the next one */
                            DeleteCommand(current_cmd);
                            current_cmd = PopList(&cmds);
                            if (current_cmd != NULL) {
                                first_arg = PopList(&current_cmd->argv);
                            }
                        }
                        /* Wait for cmd(s) to finish executing */
                        int wpid, wstatus;
                        while ((wpid = wait(&wstatus)) > 0);
                    }
                } else {
                    DeleteCommand(current_cmd);
                }
            } else {
            }

            ResetCommandList(&cmds);
        } else {
            free(line);
            if (mode == BATCH_MODE) {
                /* EOF, exit gracefully */
                ResetPath(&path);
                exit(0);
            }
        }
    }
#endif
}
