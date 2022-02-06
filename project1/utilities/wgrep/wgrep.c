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
    char *strstr_ret;
    char *needle = NULL;

    switch (argc) {
        case 1: //Error
            fprintf(stdout, "wgrep: searchterm [file ...]\n");
            exit(1);
        case 2: //read from stdin
            in_stream = stdin;
            needle = argv[1];
            break;
        case 3: //read from file
            needle = argv[1];
            break;
        default:
            break;
    };

    if (in_stream == stdin) {
        while ((nread = getline(&line, &len, in_stream)) != -1) {
            strstr_ret = strstr(line, needle);
            if (strstr_ret) {
                fprintf(stdout, "%s", line);
            }
        }

        free(line);
    } else {
        for (uint32_t i = 2; i < argc; i++) {
            in_stream = fopen(argv[i], "r");
            if (in_stream == NULL) {
                fprintf(stdout, "wgrep: cannot open file\n");
                exit(1);
            }

            while ((nread = getline(&line, &len, in_stream)) != -1) {
                strstr_ret = strstr(line, needle);
                if (strstr_ret) {
                    fprintf(stdout, "%s", line);
                }
            }

            free(line);
        }
    }


    return 0;
}