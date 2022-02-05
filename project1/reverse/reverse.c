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

enum _io_type_ {
	IO_RD_STDIN_WR_STDOUT = 1,
	IO_RD_ARG1_WR_STDOUT,
	IO_RD_ARG1_WR_ARG2,
	IO_ERROR
};

typedef struct _node_{
	void *data;
	uint32_t data_len;
	struct _node_ *next;
} node;

typedef struct {
	node *head;
	uint32_t count;
} stack;


static stack *create_stack(void) {
	stack *new_stack = (stack *) malloc(sizeof(stack));
	if (new_stack != NULL) {
		new_stack->head = NULL;
		new_stack->count = 0;
	}
	return new_stack;
}

static uint8_t push(stack *user_stack, char *data, uint32_t data_len) {
	node *new_node = (node *) malloc(sizeof(node));
	if (new_node != NULL) {
		new_node->data = (void *) data;
		new_node->data_len = data_len;
		new_node->next = user_stack->head;
		user_stack->head = new_node;
		user_stack->count++;
		return 1;
	}

	return 0;
}

static node *pop(stack *user_stack) {
	if (user_stack == NULL) {
		return NULL;
	}

	node *popped_node = user_stack->head;
	if (popped_node != NULL) {
		user_stack->head = popped_node->next;
		user_stack->count--;
	}

	return popped_node;
}

/* Must free data once finished */
static stack *parse_input(FILE *input) {
	/* Read lines and add them to a linkedlist */
	size_t nread = 0;
	size_t len = 0;
	char *line = NULL;
	stack *input_stack = create_stack();
	if (input_stack == NULL) {
		/* Failed to allocate memory, handle it */
		fprintf(stderr, "malloc failed\n");
		return NULL;
	}

	while ((nread = getline(&line, &len, input)) != -1) {
		char *data_buff = (char *) malloc((nread+1)*sizeof(char));
		data_buff[nread] = 0;	//null terminate the string
		if (data_buff == NULL) {
			errno = ENOMEM;
			break;
		}

		memcpy(data_buff, line, nread);

		if (push(input_stack, data_buff, nread) == 0) {
			errno = ENOMEM;
			break;
		}
	}

	if (errno == ENOMEM) {
		/* Malloc failed to allocate. Free up everything, print message
		 * and exit.
		 */
		node *temp_node;
		while (input_stack->count > 0) {
			temp_node = pop(input_stack);
			free(temp_node->data);
			free(temp_node);
		}

		free(input_stack);

		fprintf(stderr, "malloc failed\n");
		return NULL;
	} else {
		return input_stack;
	}
}

static uint8_t check_same(char *f1, char*f2) {
	struct stat statf1;
	struct stat statf2;
	lstat(f1, &statf1);
	lstat(f2, &statf2);
	printf("files are same? %ld,%ld\n", statf1.st_ino, statf2.st_ino);
	return (statf1.st_ino == statf2.st_ino);
}

int main(int argc, char *argv[]) {
	FILE *in_stream = NULL;
	FILE *out_stream = NULL;

	switch (argc) {
		case IO_RD_STDIN_WR_STDOUT:
			in_stream = stdin;
			out_stream = stdout;
			break;
		case IO_RD_ARG1_WR_STDOUT:
			in_stream = fopen(argv[1], "r");
			if (in_stream == NULL) {
				fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
				exit(1);
			}
			out_stream = stdout;
			break;
		case IO_RD_ARG1_WR_ARG2:
			if (strcmp(argv[1], argv[2]) == 0) {
				fprintf(stderr, "reverse: input and output file must differ\n");
				exit(1);
			}
			in_stream = fopen(argv[1], "r");
			if (in_stream == NULL) {
				fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
				exit(1);
			}
			out_stream = fopen(argv[2], "w");
			if (out_stream == NULL) {
				fprintf(stderr, "reverse: cannot open file '%s'\n", argv[2]);
				fclose(in_stream);
				exit(1);
			}

			if (check_same(argv[1], argv[2])) {
				fprintf(stderr, "reverse: input and output file must differ\n");
				fclose(in_stream);
				fclose(out_stream);
				exit(1);				
			}
			break;
		default:
			fprintf(stderr, "usage: reverse <input> <output>\n");
			exit(1);
			break;
	};

	stack *parsed_input = parse_input(in_stream);
	if (parsed_input != NULL) {
		while (parsed_input->count > 0) {
			node *curr_line = pop(parsed_input);
			fprintf(out_stream, "%s", (char *) curr_line->data);
			free(curr_line->data);
			free(curr_line);
		}

		free(parsed_input);
	}

	if (in_stream != stdin) {
		fclose(in_stream);
	}
	if (out_stream != stdout) {
		fclose(out_stream);
	}

	return 0;
}