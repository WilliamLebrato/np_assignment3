#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    // Check if the user provided the required command-line argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ip:port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parse the IP and port from the input
    char delim[] = ":";
    char *Desthost = strtok(argv[1], delim);
    char *Destport = strtok(NULL, delim);

    // Validate the parsing of host and port
    if (Desthost == NULL || Destport == NULL) {
        fprintf(stderr, "Error: Invalid format. Use <ip:port>.\n");
        return EXIT_FAILURE;
    }

    // Convert port string to integer
    int port = atoi(Destport);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Error: Invalid port number.\n");
        return EXIT_FAILURE;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets

    // Get address info for the provided IP and port
    if (getaddrinfo(Desthost, Destport, &hints, &res) != 0) {
        perror("getaddrinfo() failed");
        return EXIT_FAILURE;
    }

    // Use the address info returned by getaddrinfo to create the socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket() failed");
        freeaddrinfo(res); // Free the linked list
        return EXIT_FAILURE;
    }

    // Optional: Set socket options
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt)) < 0) {
        perror("setsockopt() failed");
        freeaddrinfo(res);
        close(sockfd);
        return EXIT_FAILURE;
    }

    // Bind the socket to the address and port returned by getaddrinfo
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind() failed");
        freeaddrinfo(res);
        close(sockfd);
        return EXIT_FAILURE;
    }

    freeaddrinfo(res); // Free the linked list after binding

    
    close(sockfd);
    return EXIT_SUCCESS;
}
