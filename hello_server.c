#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>                                                                            
#include <netinet/in.h>
#include <arpa/inet.h>
                                                                                
#include <netdb.h>

int main() {
    struct sockaddr_in server = {
        sin_family: AF_INET,
        sin_port: htons(4000),
        sin_addr: inet_addr("192.168.2.61")
    };
    
    int socketd = socket(AF_INET, SOCK_STREAM, 0);
    bind(socketd, (struct sockaddr *) &server, sizeof(server));
    listen(socketd, 5);

	struct sockaddr_in client;
	socklen_t client_len = sizeof(client);
    int connectiond = accept(socketd, (struct sockaddr *)&client, &client_len);
    close(socketd);

    write(connectiond, "Hello", 5);
    close(connectiond);
}