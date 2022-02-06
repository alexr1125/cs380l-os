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
    char curr_char;
    char prev_char;
    uint32_t num_char = 0;
    uint32_t unread_files = argc - 1;
    uint32_t curr_file;

    if (unread_files == 0) {
        fprintf(stdout, "wzip: file1 [file2 ...]\n");
        exit(1);
    }

    curr_file = 1;
    in_stream = fopen(argv[curr_file], "r");
    unread_files--;
    if (in_stream == NULL) {
        fprintf(stdout, "wcat: cannot open file\n");
        exit(1);
    }

    while(1) {
        if (fread(&curr_char, 1, 1, in_stream) != 1) {
            if (unread_files > 0) {
                fclose(in_stream);
                curr_file++;
                unread_files--;
                in_stream = fopen(argv[curr_file], "r");
            } else {
                /* End of all files. Print the last consecutive charcters */
                fwrite(&num_char, 4, 1, stdout);
                fwrite(&prev_char, 1, 1, stdout);
                break;
            }
        } else {
            if (num_char == 0) {
                /* First char read. Initialize everything */
                num_char = 1;
                prev_char = curr_char;
            } else {
                if (curr_char == prev_char) {
                    num_char++;
                } else {
                    /*Print current consecutive chararacters and reset variables */
                    fwrite(&num_char, 4, 1, stdout);
                    fwrite(&prev_char, 1, 1, stdout);
                    prev_char = curr_char;
                    num_char = 1;
                }
            }
        }
    }
    return 0;
}