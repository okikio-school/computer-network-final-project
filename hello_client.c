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
    int sucess = connect(socketd, (struct sockaddr *)&server, sizeof(server));
    if (sucess == -1) {
        printf("Failed to connect\n");
        return 1;
    }
    
    char buffer[5];
    int bytes_read = read(socketd, buffer, 5);
    if (bytes_read > 0) {
        printf("%s\n", buffer);
    }
    close(socketd);
}