/* udp_client.c - main */

#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>

#define BUFLEN 100 /* buffer length		*/

#define DATA_PDU 'D'
#define FINAL_PDU 'F'
#define ERROR_PDU 'E'
#define FILENAME_PDU 'C'

struct pdu {
    char type;
    char data[BUFLEN];
};

void set_pdu_data(struct pdu *pdu, const char *data) {
    strncpy(pdu->data, data, BUFLEN - 1);
    pdu->data[BUFLEN - 1] = '\0';
}

int receiveFile(int socketd, struct sockaddr_in *sin) {
    struct pdu received_pdu, send_pdu;
    char filename[BUFLEN];
    FILE *file;

    // Get the filename from the user
    printf("Enter filename: ");
    fgets(filename, BUFLEN, stdin);
    filename[strcspn(filename, "\n")] = '\0';  // Remove newline

    // Prepare FILENAME_PDU to request the file
    send_pdu.type = FILENAME_PDU;
    set_pdu_data(&send_pdu, filename);

    // Send filename PDU to server
    if (write(socketd, &send_pdu, sizeof(send_pdu)) < 0) {
        fprintf(stderr, "sendto failed (couldn't send filename PDU to server)\n");
        return -1;
    }

    // Open file for writing
    file = fopen("received_by_client.txt", "w");
    if (!file) {
        fprintf(stderr, "Cannot open file to write");
        return -1;
    }

    // Receive file contents
    while (1) {
        ssize_t n = read(socketd, &received_pdu, sizeof(received_pdu));
        if (n < 0) {
            fprintf(stderr, "read failed");
            fclose(file);
            return -1;
        }

        // Check PDU type
        if (received_pdu.type == ERROR_PDU) {
            fprintf(stderr, "Error from server: %s\n", received_pdu.data);
            fclose(file);
            return -1;
        } else if (received_pdu.type == DATA_PDU || received_pdu.type == FINAL_PDU) {
            fwrite(received_pdu.data, sizeof(char), n - sizeof(received_pdu.type), file);  // write data excluding the type
            if (received_pdu.type == FINAL_PDU) {
                break;  // last packet
            }
        } else {
            fprintf(stderr, "Unknown PDU type received\n");
            fclose(file);
            return -1;
        }
    }

    printf("File transfer complete.\n");
    fclose(file);
    return 0;
}

/*------------------------------------------------------------------------
 * main - UDP client for TIME service that prints the resulting time
 *------------------------------------------------------------------------
 */
int main(int argc, char **argv) {
	char *host = "localhost";
	int port = 3000;
	char now[100];			/* 32-bit integer to hold time	*/
	struct hostent *phe;	/* pointer to host information entry	*/
	struct sockaddr_in sin; /* an Internet endpoint address		*/
	int s, n, type;			/* socket descriptor and socket type	*/

	switch (argc) {
		case 1:
			break;
		case 2:
			host = argv[1];
		case 3:
			host = argv[1];
			port = atoi(argv[2]);
			break;
		default:
			fprintf(stderr, "usage: UDPtime [host [port]]\n");
			exit(1);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	/* Map host name to IP address, allowing for dotted decimal */
	if (phe = gethostbyname(host)) {
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	} else if ((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
		fprintf(stderr, "Can't get host entry \n");

	/* Allocate a socket */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		fprintf(stderr, "Can't create socket \n");

	/* Connect the socket */
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "Can't connect to %s %s \n", host, "Time");

	while (1) {
		if (receiveFile(s, &sin) < 0)
			fprintf(stderr, "Error transferring file\n");

	}
	exit(0);
}
