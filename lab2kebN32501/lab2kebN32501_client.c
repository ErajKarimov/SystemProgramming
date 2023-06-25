//
// Client. Uses UDP instead of TCP.
//
// (c) Eraj Karimov, 2023
// This source is licensed under CC BY-NC 4.0
// (https://creativecommons.org/licenses/by-nc/4.0/)
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Define a macro to check the return value of a function call.
// If negative, print an error message and exit with failure status.

#define CHECK_RESULT(res, msg)            \
do {                                    \
    if (res < 0) {                        \
        perror(msg);                    \
        exit(EXIT_FAILURE);                \
    }                                    \
} while (0)

// Define a constant buffer size.
#define BUF_SIZE        1024

// Declare a variable to store the name of the program.
char *program_name;

// Declare a struct to hold IP address and port number options.
struct opt_vars {
    char *ip;
    int port;
};

// Display help information.
void help();

// Display version information.
void version();

// Check command-line arguments for options and values.
bool check_args(int argc, char **argv, struct opt_vars *opt_flags);

// The main function of the program.
int main(int argc, char **argv) {
    // Declare variables.
    int clientSocket;
    program_name = argv[0];
    char buffer[BUF_SIZE] = {0};
    struct sockaddr_in serverAddr = {0};
    struct opt_vars opt_flags = {"127.0.0.1", 19996};
    
    // Check command-line arguments.
    if (!check_args(argc, argv, &opt_flags)) {
        exit(EXIT_SUCCESS);
    }
    
    // Open a socket connection with UDP.
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_RESULT(clientSocket, "socket");
    
    // Parse the IP address and set the server address and port number.
    if (inet_pton(AF_INET, opt_flags.ip, &(serverAddr.sin_addr)) <= 0) {
        printf("Error: Invalid IP address.\n");
        return 1;
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(opt_flags.port);
    serverAddr.sin_addr.s_addr = inet_addr(opt_flags.ip);
	
    // Prompt user to enter data to send to server.	
    printf("Data sent: ");
    char *check = fgets(buffer, sizeof(buffer), stdin);
    if (check == NULL) {
        perror("fgets");
        exit(EXIT_FAILURE);
    }
    char *pos = strchr(buffer, '\n');
    if (pos) *pos = '\0';

    // Send data to the server using UDP and wait for a response.
    int res = sendto(clientSocket, buffer, strlen(buffer), 0,
                     (const struct sockaddr *) &serverAddr, sizeof(serverAddr));
    CHECK_RESULT(res, "sendto");
    memset(buffer, 0, sizeof(buffer));
    res = recvfrom(clientSocket, buffer, sizeof(buffer), 0, NULL, NULL);
    CHECK_RESULT(res, "recvfrom");

    // Print out the response and close the socket.
    printf("Data received (%d bytes): %s\n", (int) res, buffer);
    close(clientSocket);
    return 0;
}

// Display help information.
void help() {
    printf("Usage: %s [OPTION]...\n", program_name);
    printf("Options:\n");
    printf(" -h, display this help and exit\n");
    printf(" -v, output version information and exit\n");
    printf(" -a, set custom IP address\n");
    printf(" -p , set custom port address \n");
}

// Display version information.
void version() {
    printf("%s version 1.0\n", program_name);
    printf("info: A client that sends a pair of numbers to the server and receives calculated area in response\n");
    printf("Author: Karimov Eraj\n");
    printf("Group: N32501\n");
    printf("Lab: 2 variant 31\n");
}

// Check command-line arguments for options and values.
bool check_args(int argc, char **argv, struct opt_vars *opt_flags) {
    int option;
    while ((option = getopt(argc, argv, "hva:p:")) != -1) {
        switch (option) {
            case 'a':
                if (optarg != NULL) {
                    opt_flags->ip = optarg;
                    if (getenv("LAB2DEBUG")) {
                        printf("IP : %s\n", opt_flags->ip);
                    }
                }
                break;
            case 'p':
                if (optarg != NULL) {
                    opt_flags->port = atoi(optarg);
                    if (getenv("LAB2DEBUG")) {
                        printf("PORT: %d\n", opt_flags->port);
                    }
                }
                break;
            case 'h':
                help();
                return false;
            case 'v':
                version();
                return false;
            case '?':
                printf("Incorrect argument, use --help");
                return false;
        }
    }
    return true;
}
