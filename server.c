#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>  
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 269
#define NICKNAME_MAX_LEN 12


struct Client {
    int server_socket;
    char nickname[13];
    int is_active;
};

struct RecievedMessage {
    int sender_socket;
    int bytes_received;
};

struct Client clients[SOMAXCONN];

void initialize_clients() {
    for (int i = 0; i < SOMAXCONN; i++) {
        clients[i].is_active = 0;
        clients[i].server_socket = -1;
    }
}


// Add a client to the first available slot and store the nickname
int add_client(int client_socket, const char *nickname) {
    for (int i = 0; i < SOMAXCONN; i++) {
        if (clients[i].is_active == 0) {  // Find the first inactive slot
            clients[i].is_active = 1;     // Mark the client as active
            clients[i].server_socket = client_socket;

            // Store the client's nickname
            strncpy(clients[i].nickname, nickname, sizeof(clients[i].nickname) - 1);
            clients[i].nickname[sizeof(clients[i].nickname) - 1] = '\0'; // Ensure null termination

            return 1;  // Success
        }
    }
    return 0;  // No available slot
}


// Function to validate the client's nickname when they first connect
int validate_client(int client_socket) {
    char buf[BUFFER_SIZE];
    int numbytes;

    //send "Hello 1"

    if (send(client_socket, "HELLO 1\n", 8, 0) == -1) {
        perror("send");
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    if ((numbytes = recv(client_socket, buf, sizeof(buf) - 1, 0)) <= 0) {
        if (numbytes == 0) {
            printf("Client disconnected before sending a nickname.\n");
        } else {
            perror("recv");
        }
        return -1;
    }

    if (strncmp(buf, "NICK ", 5) != 0) {
        printf("Invalid protocol message from client. Expected 'NICK <nickname>'.\n");
        send(client_socket, "ERROR Invalid nickname format\n", 29, 0);
        return -1; // Invalid message format
    }

    char *nickname = buf + 5; 
    nickname[strcspn(nickname, "\n")] = '\0'; 

    if (strlen(nickname) > NICKNAME_MAX_LEN || strspn(nickname, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") != strlen(nickname)) {
        printf("Invalid nickname received: %s\n", nickname);
        send(client_socket, "ERROR Invalid nickname\n", 23, 0);
        return -1; 
    }

    send(client_socket, "OK\n", 3, 0);

    add_client(client_socket, nickname);  
    return 0;
}


void remove_client(int client_socket) {
    for (int i = 0; i < SOMAXCONN; i++) {
        if (clients[i].server_socket == client_socket && clients[i].is_active) {
            clients[i].is_active = 0;
            close(clients[i].server_socket);
            clients[i].server_socket = -1;
            break;
        }
    }
}

struct ReceivedMessage {
    int sender_socket;     // The socket of the client who sent the message
    int bytes_received;    // Number of bytes received, or -1 if there was a disconnection/error
};

// Function to receive a message and return the result in a struct
struct ReceivedMessage receive_message(fd_set *master_fds, fd_set *read_fds, int fdmax, int server_socket, char *buffer) {
    struct ReceivedMessage result;
    result.sender_socket = -1;    // Default value indicating no sender
    result.bytes_received = -1;   // Default value indicating error/no data received

    // Loop through all the file descriptors to check which one is ready to send data
    for (int i = 0; i <= fdmax; i++) {
        if (i != server_socket && FD_ISSET(i, read_fds)) {
            // Receive the message from the client
            int numbytes = recv(i, buffer, BUFFER_SIZE, 0);
            
            if (numbytes <= 0) {
                if (numbytes == 0) {
                    printf("Client socket %d disconnected.\n", i);
                } else {
                    // If recv returns -1, there was an error receiving the message
                    perror("recv failed");
                }

                close(i);
                FD_CLR(i, master_fds);
                remove_client(i);

                result.sender_socket = i;
                result.bytes_received = -1; // Disconnected or error
                return result;  // Return the result
            } else {
                buffer[numbytes] = '\0';  // Null-terminate the buffer for safety

                result.sender_socket = i;
                result.bytes_received = numbytes;  // Number of bytes received
                return result;  // Return the result
            }
        }
    }

    return result;
}



void broadcast(int sender_server_socket, const char *message) {
    for (int i = 0; i < SOMAXCONN; i++) {
        if (clients[i].is_active && clients[i].server_socket != sender_server_socket) {
            if (send(clients[i].server_socket, message, strlen(message), 0) == -1) {
                perror("send failed");
                remove_client(clients[i].server_socket);
            }
        }
    }
}



int main(int argc, char *argv[]) {
    // Check if the user provided the required command-line argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ip:port>\n", argv[0]);
        return -1;
    }

    // Parse the IP and port from the input
    char delim[] = ":";
    char *Desthost = strtok(argv[1], delim);
    char *Destport = strtok(NULL, delim);

    // Validate the parsing of host and port
    if (Desthost == NULL || Destport == NULL) {
        fprintf(stderr, "Error: Invalid format. Use <ip:port>.\n");
        return -1;
    }

    // Convert port string to integer
    int port = atoi(Destport);
    if (port <= 0) {
        fprintf(stderr, "Error: Invalid port number.\n");
        return -1;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets

    if (getaddrinfo(Desthost, Destport, &hints, &res) != 0) {
        perror("getaddrinfo() failed");
        return -1;
    }

    int server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_socket < 0) {
        perror("socket() failed");
        freeaddrinfo(res); // Free the linked list
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt)) < 0) {
        perror("setsockopt() failed");
        freeaddrinfo(res);
        close(server_socket);
        return -1;
    }

    // Bind the socket to the address and port returned by getaddrinfo
    if (bind(server_socket, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind() failed");
        freeaddrinfo(res);
        close(server_socket);
        return -1;
    }

    freeaddrinfo(res); // Free the linked list after binding

    if (listen(server_socket, SOMAXCONN) == -1) {
        perror("listen() failed");
        close(server_socket);
        return -1;
    }


    fd_set master_fds, read_fds;

    FD_ZERO(&master_fds);  
    FD_ZERO(&read_fds);

    FD_SET(server_socket, &master_fds);

    int fdmax = server_socket;
    int client_socket;
    int bytes_received;

    char buffer[BUFFER_SIZE];
    initialize_clients();
    while(1)
    {
        
        read_fds = master_fds;

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select() failed");
            exit(EXIT_FAILURE);
        }

        // check if the server socket is set
        if (FD_ISSET(server_socket, &read_fds)) {
            struct sockaddr_storage client_addr;
            socklen_t addr_size = sizeof(client_addr);
            if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size)) < 0) {
                perror("accept() failed");
                continue;
            }

            if (validate_client(client_socket) == -1) {
                close(client_socket);
                continue;
            }

            FD_SET(client_socket, &master_fds);

            if (client_socket > fdmax) {
                fdmax = client_socket;
            }
        }

        else
        {
            struct ReceivedMessage received_message = receive_message(&master_fds, &read_fds, fdmax, server_socket, buffer);
            client_socket = received_message.sender_socket;
            bytes_received = received_message.bytes_received;

            if (bytes_received == -1) {
                continue;
            }

            if (strncmp(buffer, "MSG ", 4) == 0) {
                broadcast(client_socket, buffer);
            } 
        }
    }

    close(server_socket);  
    return EXIT_SUCCESS;
}
