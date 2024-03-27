/* A simple file transfer server using TCP */
#include <stdio.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define SERVER_TCP_PORT 3000	/* well-known port */
#define BUFLEN		256	/* buffer length */

int transferFile(int);
void reaper(int);

int main(int argc, char **argv) {
	int 	sd, new_sd, client_len, port;
	struct	sockaddr_in server, client;

	switch(argc){
	case 1:
		port = SERVER_TCP_PORT;
		break;
	case 2:
		port = atoi(argv[1]);
		break;
	default:
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	/* Bind an address to the socket	*/
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	/* queue up to 5 connect requests  */
	listen(sd, 5);

	(void) signal(SIGCHLD, reaper);

	while(1) {
		client_len = sizeof(client);
		new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
		if(new_sd < 0) {
			fprintf(stderr, "Can't accept client \n");
			exit(1);
		}
		switch (fork()) {
		case 0:		/* child */
			(void) close(sd);
			exit(transferFile(new_sd));
		default:		/* parent */
			(void) close(new_sd);
			break;
		case -1:
			fprintf(stderr, "fork: error\n");
		}
	}
}

/*	file transfer function	*/
int transferFile(int sd) {
	char	*fileContents = 0, *bp, filename[BUFLEN], outgoing[BUFLEN];
	int 	n, bytes_to_read, fileLength;
	FILE 	*fptr;
	long	length;

	while(n = read(sd, filename, BUFLEN)) {
		fptr = fopen(filename, "rb");
		break;
	}
	if (fptr == NULL) {
		write(sd, "eError: File does not exist.\n", n);
	}
	else {
		fseek (fptr, 0, SEEK_END);
		length = ftell(fptr);
		fseek (fptr, 0, SEEK_SET);
		fileContents = malloc(length);
		if (fileContents) {
			fread(fileContents, 1, length, fptr);
		}
		snprintf(outgoing, sizeof outgoing, "f%s", fileContents);
		fileLength = strlen(outgoing);
		for (int i = 0; i < fileLength; i += 100) {
			memmove(outgoing, outgoing+i, fileLength);
			write(sd, outgoing, 100);
		}	
	}
	return(0);
}

/*	reaper		*/
void	reaper(int sig) {
	int	status;
	while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}
