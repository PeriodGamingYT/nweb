// Jaocb Sorber, "Program your own web server in C. (sockets)".
// Heavily Modified.
#define LOG
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/time.h>
#include <netdb.h>

typedef struct {
	char **args;
	int arg_size;
} args_t;

args_t split_string(char *bad_string, char *delim) {
	args_t result = {
		NULL,
		0
	};

	// copied because gcc makes bad_string read-only, resulting in a 
	// segfault, this circumvents this problem.
	char *string = (char *) malloc(sizeof(char) * strlen(bad_string));
	memcpy(string, bad_string, sizeof(char) * strlen(bad_string));
	char *token = strtok(string, delim);
	while(token != NULL && token[0] != '\0') {
		int token_size = strlen(token);
		char *copy_token = (char *) malloc(sizeof(char) * (token_size + 1));
		memcpy(copy_token, token, sizeof(char) * token_size);
		copy_token[token_size] = '\0';
		result.arg_size++;
		result.args = (char **) realloc(
			result.args,
			result.arg_size * sizeof(char *)
		);

		result.args[result.arg_size - 1] = copy_token;
		token = strtok(NULL, delim);
	}

	free(string);
	return result;
}

void die(const char *message) {
	perror(message);
	exit(1);
}

void free_args(args_t *args) {
	for(int i = 0; i < args->arg_size; i++) {
		free(args->args[i]);
	}

	free(args->args);
}

char *file_dump(char *file_name) {
	if(file_name == NULL) {
		return NULL;
	}

	char *result = malloc(0);
	int result_size = 0;
	FILE *file = fopen(file_name, "r");
	if(file == NULL) {
		return NULL;
	}
	
	while(!feof(file)) {
		char new_char = fgetc(file);
		if(new_char == 0) {
			continue;
		}

		if(feof(file)) {
			break;
		}
		
		result = (char *) realloc(result, ++result_size);
		result[result_size - 1] = new_char;
	}

	fclose(file);
	return result;
}

#define PORT 18000
#define MAX_LINE 4096
#define MAX_LINES 1024
int main(int argc, char **argv) {
	if(argc < 2) {
		die("usage: nweb directory");
	}

	char *directory = argv[1];

	// dirty hack.
	if(directory[strlen(directory) - 1] == '/') {
		directory[strlen(directory) - 1] = 0;
	}
	
	int listen_fd, conn_fd, n;
	struct sockaddr_in server_addr;
	uint8_t buffer[MAX_LINE + 1];
	uint8_t recv_line[MAX_LINE + 1];
	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		die("can't open socket");
	}

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);
	if(bind(
		listen_fd, 
		(struct sockaddr *) &server_addr, 
		sizeof(server_addr)
	) < 0) {
		die("can't bind");
	}

	if((listen(listen_fd, 10)) < 0) {
		die("can't listen");
	}

	char recv_lines[MAX_LINES][MAX_LINE] = { 0 };
	int recv_n = 0;
	for(;;) {
		#ifdef LOG
			printf("waiting for conn on port %d\n", PORT);
		#endif
		
		fflush(stdout);
		conn_fd = accept(listen_fd, (struct sockaddr *) NULL, NULL);
		memset(recv_line, 0, MAX_LINE);
		while((n = read(conn_fd, recv_line, MAX_LINE - 1)) > 0) {
			memcpy(recv_lines[recv_n++], recv_line, MAX_LINE);
			if(recv_line[n - 1] == '\n') {
				break;
			}

			memset(recv_line, 0, MAX_LINE);
		}

		if(n < 0) {
			die("can't read");
		}

		args_t request_args = split_string(recv_lines[0], " ");
		char *request = NULL;	
		if(strcmp(request_args.args[0], "GET") == 0) {
			int length = strlen(directory) + strlen(request_args.args[1]) + 1;
			request = (char *) malloc(length);
			memset(request, 0, length);
			memcpy(request, directory, strlen(directory));
			memcpy(
				&request[strlen(directory)], 
				request_args.args[1], 
				strlen(request_args.args[1])
			);
			
			request[length - 1] = 0;
			#ifdef LOG
				printf("directory %s\n", request);
			#endif
		}

		char *dump = file_dump(request);
		if(request != NULL) {
			free(request);
		}
		
		snprintf(
			(char *) buffer,
			sizeof(buffer), 
			"HTTP/1.0 200 OK\r\n\r\n%s",
			dump
		);

		if(dump != NULL) {
			free(dump);
		}
		
		free_args(&request_args);
		write(conn_fd, (char *) buffer, strlen((char *) buffer));
		close(conn_fd);
	}
	
	return 0;
}
