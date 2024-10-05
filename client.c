#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>


#define BUFFER_SIZE 1024




int main(int argc, char *argv[]){
  
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_address:port> <nickname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse the server address and port from the first argument
    char delim[] = ":";
    char *Desthost = strtok(argv[1], delim);
    char *Destport = strtok(NULL, delim);

    if (!Destport) {
        fprintf(stderr, "Error: Invalid format. Use <server_address:port>.\n");
        return -1;
    }

    char *nickname = argv[2];

    // Validate the nickname (12 characters max, only A-Za-z0-9_)
    if (strlen(nickname) > 12 || strspn(nickname, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") != strlen(nickname)) {
        fprintf(stderr, "Invalid nickname. Must be up to 12 characters long and can contain A-Z, a-z, 0-9, and _.\n");
        return -1;
    }

    // Set up the socket
    int sockfd;
    int rv;
    char buf[BUFFER_SIZE];
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets


    if ((rv = getaddrinfo(Desthost, Destport, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("connect");
            continue;
        }

        break;  // Successfully connected
    }
    freeaddrinfo(servinfo);

    if(1)   // PROTOCL VERSION SECTION

        {
        memset(buf, 0, sizeof buf); // Clear the buffer
        int numbytes;
        if ((numbytes = recv(sockfd, buf, BUFFER_SIZE - 1, 0)) == -1) {
            perror("recv");
            close(sockfd); // Ensure the socket is closed on error
            return -1; // Return with error status
        }
        printf("Received: %s\n", buf);
        if (strstr(buf, "HELLO 1") == NULL ) {
            printf("ERROR\n");  // Match expected output
            close(sockfd);      // Close the socket immediately
            return 1;           // Exit the client to avoid timeout
        }
        char message[20];
        snprintf(message, sizeof(message), "NICK %s\n", nickname);
        if (send(sockfd, message, strlen(message), 0) == -1) {
            perror("send");
            close(sockfd);
            return -1;
        }
        }


    if (1) // NICKNAME SECTION
    {
        memset(buf, 0, sizeof buf);
        if (recv(sockfd, buf, BUFFER_SIZE - 1, 0) == -1) {
            perror("recv");
            close(sockfd);
            return -1;
        }

        printf("Nickname response: %s\n", buf);
        // Check if the server accepted the nickname
        if (strstr(buf, "OK") != NULL) 
        {
            printf("Server accepted nickname: %s\n", nickname);    
        }
        else if (strstr(buf, "ERROR") != NULL) 
        {
            printf("ERROR: Nickname not accepted by the server\n");
            close(sockfd);
            return -1;
        }
        else
        {
            printf("Unknown Response from server: %s\n", buf);
            close(sockfd);
            return -1;
        }
    }

    while(1) // Main Loop
    {

    }
    
    return 0;
}
