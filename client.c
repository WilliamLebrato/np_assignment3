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

#define BUFFER_SIZE 300

// ./cchat 13.53.76.30:4711 William

int main(int argc, char *argv[]){
  
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_address:port> <nickname>\n", argv[0]);
        fflush(stderr);  // Flush stderr after error print
        exit(EXIT_FAILURE);
    }

    // Parse the server address and port from the first argument
    char delim[] = ":";
    char *Desthost = strtok(argv[1], delim);
    char *Destport = strtok(NULL, delim);

    if (!Destport) {
        fprintf(stderr, "Error: Invalid format. Use <server_address:port>.\n");
        fflush(stderr);  // Flush stderr after error print
        return -1;
    }

    char *nickname = argv[2];

    // Validate the nickname (12 characters max, only A-Za-z0-9_)
    if (strlen(nickname) > 12 || strspn(nickname, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") != strlen(nickname)) {
        fprintf(stderr, "Invalid nickname. Must be up to 12 characters long and can contain A-Z, a-z, 0-9, and _.\n");
        fflush(stderr);  // Flush stderr after error print
        fprintf(stderr, "ERROR\n");  // Print "ERROR" as required by the test
        fflush(stderr);  // Flush stderr after error print
        exit(-1);  // Exit with the correct non-zero value (255) as expected by the test
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
        fflush(stderr);  // Flush stderr after error print
        return -1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            fflush(stderr);  // Flush stderr after error print
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("connect");
            fflush(stderr);  // Flush stderr after error print
            continue;
        }

        break;  // Successfully connected
    }

    if (p == NULL) 
    {
        fprintf(stderr, "connect: Connection refused\n");
        fflush(stderr);  // Flush stderr after error print
        fprintf(stderr, "ERROR\n");  // Ensure "ERROR" is printed
        fflush(stderr);  // Flush stderr after error print
        exit(EXIT_FAILURE);         // Exit with a non-zero status
    }

    freeaddrinfo(servinfo);

    if(1)   // PROTOCOL VERSION SECTION
    {
        memset(buf, 0, sizeof buf); // Clear the buffer
        int numbytes;
        if ((numbytes = recv(sockfd, buf, BUFFER_SIZE - 1, 0)) == -1) {
            perror("recv");
            fflush(stderr);  // Flush stderr after error print
            close(sockfd); // Ensure the socket is closed on error
            return -1; // Return with error status
        }
        printf("Received: %s\n", buf);
        fflush(stdout);  // Flush stdout after print
        if (strstr(buf, "HELLO 1") == NULL ) {
            printf("ERROR\n");  // Match expected output
            fflush(stdout);  // Flush stdout after print
            close(sockfd);      // Close the socket immediately
            return 1;           // Exit the client to avoid timeout
        }
        printf("Protocol Supported\n");
        fflush(stdout);  // Flush stdout after print
        char message[20];
        snprintf(message, sizeof(message), "NICK %s\n", nickname);
        if (send(sockfd, message, strlen(message), 0) == -1) {
            perror("send");
            fflush(stderr);  // Flush stderr after error print
            close(sockfd);
            return -1;
        }
    }

    if (1) // NICKNAME SECTION
    {
        memset(buf, 0, sizeof buf);
        if (recv(sockfd, buf, BUFFER_SIZE - 1, 0) == -1) {
            perror("recv");
            fflush(stderr);  // Flush stderr after error print
            close(sockfd);
            return -1;
        }

        printf("Nickname response: %s\n", buf);
        fflush(stdout);  // Flush stdout after print
        // Check if the server accepted the nickname
        if (strstr(buf, "OK") != NULL) 
        {
            printf("Server accepted nickname: %s\n", nickname);    
            fflush(stdout);  // Flush stdout after print
        }
        else
        {
            printf("Unknown Response from server: %s\n", buf);
            fflush(stdout);  // Flush stdout after print
            close(sockfd);
            return -1;
        }
    }

    if (fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
        fflush(stderr);  // Flush stderr after error print
        close(sockfd);
        return -1;
    }

    fd_set read_fds;
    int max_fd;


    char recv_buffer[BUFFER_SIZE * 2]; // Double size to handle concatenated messages
    int recv_buffer_len = 0;   

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
            fflush(stderr);  // Flush stderr after error print
            continue;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) // SEND DATA
        {
            memset(buf, 0, sizeof buf); // Clear the buffer
            int numbytes;
            if ((numbytes = read(STDIN_FILENO, buf, BUFFER_SIZE - 1)) == -1) {
                perror("read");
                fflush(stderr);  // Flush stderr after error print
                continue;
            }

            if (numbytes == 0) // Empty input
            {
                printf("EOF\n");
                fflush(stdout);  // Flush stdout after print
                continue;
            }

            // Create a new buffer to hold "MSG " + message
            char message_buffer[BUFFER_SIZE + 5]; 
            snprintf(message_buffer, sizeof(message_buffer), "MSG %s", buf);  

            if (send(sockfd, message_buffer, strlen(message_buffer), 0) == -1) {
                perror("send");
                fflush(stderr);  // Flush stderr after error print
                continue;
            }
        }

        if (FD_ISSET(sockfd, &read_fds)) // RECEIVE DATA
            {
                int numbytes_recv;

                // Receive data and append to the persistent buffer
                if ((numbytes_recv = recv(sockfd, recv_buffer + recv_buffer_len, sizeof(recv_buffer) - recv_buffer_len - 1, 0)) <= 0) {
                    if (numbytes_recv == 0) {
                        // Server closed connection
                        printf("Server closed the connection.\n");
                        fflush(stdout);  // Flush stdout after print
                    } else {
                        perror("recv");
                        fflush(stderr);  // Flush stderr after error print
                    }
                    close(sockfd);
                    return -1;
                }

                recv_buffer_len += numbytes_recv;
                recv_buffer[recv_buffer_len] = '\0'; // Null-terminate for string operations

                // Process complete messages in the buffer
                char *line_start = recv_buffer;
                char *newline_pos;
                while ((newline_pos = strchr(line_start, '\n')) != NULL) {
                    *newline_pos = '\0'; // Null-terminate the message

                    // Process the message pointed to by line_start
                    if (strncmp(line_start, "MSG ", 4) == 0) {
                        char *msg_start = line_start + 4; 

                        char *nickname_end = strchr(msg_start, ' ');
                        if (nickname_end != NULL) {
                            *nickname_end = '\0'; 

                            char *sender_nick = msg_start;
                            char *message_text = nickname_end + 1; 

                            // Compare sender's nickname with your own
                            if (strcmp(sender_nick, nickname) != 0) {
                                printf("%s: %s\n", sender_nick, message_text); 
                                fflush(stdout);  // Flush stdout after print
                            }
                            // Else, it's your own message echoed back; do not print
                        } else {
                            printf("Malformed message from server: %s\n", line_start);
                            fflush(stdout);  // Flush stdout after print
                        }
                    }
                    else if (strncmp(line_start, "ERROR", 5) == 0 || strncmp(line_start, "ERR", 3) == 0) {
                        printf("%s\n", line_start);
                        fflush(stdout);  // Flush stdout after print
                    }
                    else {
                        printf("Unknown message from server: %s\n", line_start);
                        fflush(stdout);  // Flush stdout after print
                    }

                    // Move to the next line
                    line_start = newline_pos + 1;
                }

                // Move any partial message to the beginning of the buffer
                recv_buffer_len = strlen(line_start);
                memmove(recv_buffer, line_start, recv_buffer_len);
                recv_buffer[recv_buffer_len] = '\0'; // Null-terminate the buffer
            }
    }
    return 0;
}
