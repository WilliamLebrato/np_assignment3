#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>  
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define buff_size 270

struct Client {
    int server_socket;
    char ip_address[INET6_ADDRSTRLEN];
    char nickname[13];
    int is_active;
};

struct Client clients[SOMAXCONN];

void initialize_clients() {
    for (int i = 0; i < SOMAXCONN; i++) {
        clients[i].is_active = 0;
        clients[i].server_socket = -1;
    }
}

// Add a client to the first available slot and store the nickname
int add_client(int client_server_socket, const char *nickname) {
    for (int i = 0; i < SOMAXCONN; i++) {
        if (clients[i].is_active == 0) {  // Find the first inactive slot
            clients[i].is_active = 1;     // Mark the client as active
            clients[i].server_socket = client_server_socket;

            // Store the client's nickname
            strncpy(clients[i].nickname, nickname, sizeof(clients[i].nickname) - 1);
            clients[i].nickname[sizeof(clients[i].nickname) - 1] = '\0'; // Ensure null termination

            return 1;  // Success
        }
    }
    return 0;  // No available slot
}

void remove_client(int client_server_socket) {
    for (int i = 0; i < SOMAXCONN; i++) {
        if (clients[i].server_socket == client_server_socket && clients[i].is_active) {
            clients[i].is_active = 0;
            close(clients[i].server_socket);
            clients[i].server_socket = -1;
            printf("Client %s disconnected.\n", clients[i].ip_address);
            break;
        }
    }
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

    char buffer[buff_size]

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets

    // Get address info for the provided IP and port
    if (getaddrinfo(Desthost, Destport, &hints, &res) != 0) {
        perror("getaddrinfo() failed");
        return -1;
    }

    // Use the address info returned by getaddrinfo to create the socket
    int server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_socket < 0) {
        perror("socket() failed");
        freeaddrinfo(res); // Free the linked list
        return -1;
    }

    // Optional: Set socket options
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
    int fdmax = server_socket;  // Initial max is the server socket file descriptor

    // Initialize the master set
    FD_ZERO(&master_fds);  // Clear the master and temp sets
    FD_ZERO(&read_fds);

    // Add the server socket to the master set
    FD_SET(server_socket, &master_fds);  // Add server socket to fd_set

    while(1)
    {

    }

    close(server_socket);  
    return EXIT_SUCCESS;
}
