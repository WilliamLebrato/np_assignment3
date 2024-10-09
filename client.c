#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>

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
        printf("Protocol Supported\n");
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

    if (fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
        close(sockfd);
        return -1;
    }

    fd_set read_fds;
    int max_fd;

    while(1) // Main Loop
    {
        FD_ZERO(&read_fds);                 // Clear the fd_set (initialize it)
        FD_SET(STDIN_FILENO, &read_fds);    // Add stdin to the fd_set
        FD_SET(sockfd, &read_fds);          // Add the socket to the fd_set

        max_fd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;
    

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0)
        {
            perror("select");
            continue;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) // SEND DATA
        {
            memset(buf, 0, sizeof buf); // Clear the buffer
            int numbytes;
            if ((numbytes = read(STDIN_FILENO, buf, BUFFER_SIZE - 1)) == -1) {
                perror("read");
                continue;
            }

            if (numbytes == 0) // Empty input
            {
                printf("EOF\n");
                continue;
            }

            // Create a new buffer to hold "MSG " + message
            char message_buffer[BUFFER_SIZE + 5]; 
            snprintf(message_buffer, sizeof(message_buffer), "MSG %s", buf);  

            if (send(sockfd, message_buffer, strlen(message_buffer), 0) == -1) {
                perror("send");
                continue;
            }
        }

        if (FD_ISSET(sockfd, &read_fds)) // RECEIVE DATA
        {
            memset(buf, 0, sizeof buf); // Clear the buffer
            int numbytes_recv;

            if ((numbytes_recv = recv(sockfd, buf, BUFFER_SIZE - 1, 0)) == -1) {
                perror("recv");
                break;
            }

            if (numbytes_recv == 0) // Server closed connection
            {
                printf("Server closed the connection.\n");
                close(sockfd);
                return -1;
            }

            buf[numbytes_recv] = '\0'; // Ensure null-termination

            if (strncmp(buf, "MSG ", 4) == 0) {
                char *msg_start = buf + 4; 

                char *nickname_end = strchr(msg_start, ' ');
                if (nickname_end != NULL) {
                    *nickname_end = '\0'; 

                    printf("%s: ", msg_start);

                    printf("%s\n", nickname_end + 1); 
                } else {
                    printf("%s\n", buf);
                }
            }
            else if (strncmp(buf, "ERROR", 5) == 0 || strncmp(buf, "ERR", 3) == 0) {
                printf("%s", buf);
            }
            else {
                printf("Unknown message from server: %s\n", buf);
            }
        }

    }
    return 0;
}
