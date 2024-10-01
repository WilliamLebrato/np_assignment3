#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

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
        exit(EXIT_FAILURE);
    }

    char *nickname = argv[2];

    // Validate the nickname (12 characters max, only A-Za-z0-9_)
    if (strlen(nickname) > 12 || strspn(nickname, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") != strlen(nickname)) {
        fprintf(stderr, "Invalid nickname. Must be up to 12 characters long and can contain A-Z, a-z, 0-9, and _.\n");
        exit(EXIT_FAILURE);
    }


  return 0;

}
