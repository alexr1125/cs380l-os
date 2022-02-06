//Your code goes here..!
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    FILE *in_stream = NULL;
    size_t nread = 0;
    size_t len = 0;
    char *line = NULL;

    for (uint32_t i = 1; i < argc; i++) {
        in_stream = fopen(argv[i], "r");
        if (in_stream == NULL) {
            fprintf(stdout, "wcat: cannot open file\n");
            exit(1);
        }

        while ((nread = getline(&line, &len, in_stream)) != -1) {
            fprintf(stdout, "%s", line);
        }

        free(line);
    }

    return 0;
}