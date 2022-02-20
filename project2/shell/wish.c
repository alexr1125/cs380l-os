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

/* GLOBAL VARIABLES */
const char error_message[30] = "An error has occurred\n";

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

static void RunInteractiveMode(void) {
    size_t nread, len;
    char *line;

    while (1) {
        printf("wish>");
        if ((nread = getline(&line, &len, stdin)) != -1) {
            
        }
    }
}

static void RunBatchMode(char *file_path) {

}