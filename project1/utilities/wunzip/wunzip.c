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
    char line[5];

    if (argc <= 1) {
        fprintf(stdout, "wunzip: file1 [file2 ...]\n");
        exit(1);
    }

    for (uint32_t i = 1; i < argc; i++) {
        in_stream = fopen(argv[i], "r");
        if (in_stream == NULL) {
            fprintf(stdout, "wcat: cannot open file\n");
            exit(1);
        }

        while (fread(line, 5, 1, in_stream)) {
            uint32_t num_chars = *((uint32_t *) line);
            char char_to_print = line[4];
            for(uint32_t i = 0; i < num_chars; i++) {
                fprintf(stdout, "%c", char_to_print);
            }
        }
    }

    return 0;
}