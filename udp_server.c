/* udp_server.c - main */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

#define BUFLEN 100 /* buffer length		*/
#define DATA_PDU 'D'
#define FINAL_PDU 'F'
#define ERROR_PDU 'E'
#define FILENAME_PDU 'C'

struct pdu {
	char type;
	char data[BUFLEN];
};

void set_pdu_data(struct pdu *pdu, const char *data, size_t len) {
    memcpy(pdu->data, data, len); // Directly copy the specified length of data
}

int transferFile(int socketd, const char* buf, struct sockaddr *addr, socklen_t addr_len) {
    char fileContents[BUFLEN]; // Adjusted size to match PDU data field
    struct pdu result_pdu;
    int fileLength, bytesRead = 0, bytesToRead;

    // Extract filename from received PDU
    struct pdu received_pdu;
    sscanf(buf, "%c%99s", &received_pdu.type, received_pdu.data); // Ensure null-termination
    if (received_pdu.type != FILENAME_PDU) {
		// Send error message to client
		char *error = "Error: Expected filename PDU";
		result_pdu.type = ERROR_PDU;
		set_pdu_data(&result_pdu, error, sizeof(error));
		if (sendto(socketd, &result_pdu, sizeof(char) + sizeof(error), 0, addr, addr_len) == -1) {
			fprintf(stderr, "sendto failed (when trying to send an error message): - \"%s\"\n", error);
		}

        return -1;
    }

    // Open the file
    FILE *file = fopen(received_pdu.data, "r");
    if (file == NULL) {
		// Send error message to client
		char *error = "Error: File not found";
		result_pdu.type = ERROR_PDU;
		set_pdu_data(&result_pdu, error, sizeof(error));
		if (sendto(socketd, &result_pdu, sizeof(char) + sizeof(error), 0, addr, addr_len) == -1) {
			fprintf(stderr, "sendto failed (when trying to send an error message): - \"%s\"\n", error);
		}

        return -1;
    }

	// Determine file size
    struct stat st;
    if (stat(received_pdu.data, &st) == 0) {
        fileLength = st.st_size;
    } else {
        fclose(file);
		
		// Send error message to client
		char *error = "Error: Cannot determine file size";
		result_pdu.type = ERROR_PDU;
		set_pdu_data(&result_pdu, error, sizeof(error));
		if (sendto(socketd, &result_pdu, sizeof(char) + sizeof(error), 0, addr, addr_len) == -1) {
			fprintf(stderr, "sendto failed (when trying to send an error message): - \"%s\"\n", error);
		}

        return -1;
    }

    // Read and send file contents in chunks
    while ((bytesToRead = fread(fileContents, sizeof(char), BUFLEN, file)) > 0) {
        result_pdu.type = (fileLength <= bytesRead + bytesToRead) ? FINAL_PDU : DATA_PDU;
        set_pdu_data(&result_pdu, fileContents, bytesToRead);

        if (sendto(socketd, &result_pdu, sizeof(char) + bytesToRead, 0, addr, addr_len) == -1) {
			fprintf(stderr, "sendto failed\n");
            break;
        }
        bytesRead += bytesToRead;
    }

	// Check for errors in reading the file
	int result = (ferror(file) ? -1 : 0);
    fclose(file); // Ensure file is always closed (we want to avoid memory leaks...)
    return result;
}

/*------------------------------------------------------------------------
 * main - Iterative UDP server for TIME service
 *------------------------------------------------------------------------
 */
int main(int argc, char *argv[]) {
	struct sockaddr_in fsin; /* the from address of a client	*/
	char buf[BUFLEN];		 /* "input" buffer; any size > 0	*/
	char *pts;
	int sock;				/* server socket		*/
	time_t now;				/* current time			*/
	int alen;				/* from-address length		*/
	struct sockaddr_in sin; /* an Internet endpoint address         */
	int s, type;			/* socket descriptor and socket type    */
	int port = 3000;

	switch (argc) {
		case 1:
			break;
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	/* Allocate a socket */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		fprintf(stderr, "can't creat socket\n");

	/* Bind the socket */
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "can't bind to %d port\n", port);
	listen(s, 5);
	alen = sizeof(fsin);

	while (1) {

		if (recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&fsin, &alen) < 0)
			fprintf(stderr, "recvfrom error\n");

		if (transferFile(s, buf, (struct sockaddr *)&fsin, sizeof(fsin)) < 0)
			fprintf(stderr, "- Error transferring file\n");

		// (void)sendto(s, pts, strlen(pts), 0, (struct sockaddr *)&fsin, sizeof(fsin));
	}
}