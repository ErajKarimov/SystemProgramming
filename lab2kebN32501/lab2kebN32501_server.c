//
// Server. Uses UDP instead of TCP.
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
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <stdbool.h>
#include <signal.h>

#define CHECK_RESULT(res, msg)            \
do {                                    \
    if (res < 0) {                        \
        perror(msg);                    \
        exit(EXIT_FAILURE);                \
    }                                    \
} while (0)

// Define a constant buffer size and responce size
#define BUF_SIZE        1024
#define RESPONCE_SIZE        128


// Declare global variables
char save_time_begin[20];
char *program_name;
double cpu_time = 0.0;
int error_req = 0;
int serverSocket;
int users = 0;
time_t current_time;
clock_t begin;
clock_t end;

// Define structure for optional variables
struct opt_vars {
    bool flag_ip;
    bool flag_port;
    bool flag_wait;
    bool flag_path_to_log;
    const char *path_to_log;
    const char *ip;
    int port;
    int c_sleep;
};

// Define structure for environmental variables
struct env_vars {
    char *env_debug;
    char *env_ip;
    char *env_log;
    char *env_port;
    char *env_wait;
};

// Display help information.
void help();

// Display version information.
void version();

// Check command-line arguments for options and values.
bool check_arg(int argc, char **argv, struct opt_vars *opt_flags, struct env_vars *env_var);

// Check environmental variables
void check_env(struct opt_vars *opt_flags, struct env_vars *env_var);

// Calculate area 
double calculate_area(double x1, double y1, double x2, double y2);

// Use Calulate_area for founding area
double found_area(char *buffer);

// Handle request
void handle_request(char *buffer, struct sockaddr_in *clientAddr, socklen_t len, struct opt_vars *opt_flags,
                    struct env_vars *env_var);
                    
// Handle signals
void signal_handler(int __attribute__((unused))(signum));

// Handle user-defined signals
void user_sig_handler(int __attribute__((unused))(signum));

// Check for signals
bool check_signal(struct sigaction *signal_state);

// Daemonize the program
void turn_on_daemon();

// The main function of the program.
int main(int argc, char **argv) {
    int res;
    char buffer[BUF_SIZE] = {0};
    program_name = argv[0];
    // Initialize time and socket structures
    time(&current_time);
    struct sockaddr_in serverAddr = {0};
    struct sockaddr_in clientAddr = {0};
    
    // Initialize optional and environmental variable structures
    struct opt_vars opt_flags = {false, false, false, false, "/tmp/lab2.log", "127.0.0.1", 19996, 0};
    struct env_vars env_var = {getenv("LAB2DEBUG"), getenv("LAB2ADDR"), getenv("LAB2LOGFILE"), getenv("LAB2PORT"),
                               getenv("LAB2WAIT")};
                               
    struct sigaction signal_state;
    struct sigaction user_signal_state;
    // Initialize signal states for handling signals	
    signal_state.sa_handler = signal_handler;
    sigemptyset(&signal_state.sa_mask);
    signal_state.sa_flags = 0;
    user_signal_state.sa_handler = user_sig_handler;
    sigemptyset(&user_signal_state.sa_mask);
    user_signal_state.sa_flags = 0;

    
    // Check optional and environmental variables
    if (!check_arg(argc, argv, &opt_flags, &env_var)) {
        exit(EXIT_SUCCESS);
    }
    check_env(&opt_flags, &env_var);

    srand(time(NULL));
    
    // Create UDP socket
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_RESULT(serverSocket, "socket");

    // Allow socket reuse
    res = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int));
    CHECK_RESULT(res, "setsockopt");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(opt_flags.port);
    serverAddr.sin_addr.s_addr = inet_addr(opt_flags.ip);

    res = bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    CHECK_RESULT(res, "bind");

    // Check for signals
    if (check_signal(&signal_state)) {
        exit(EXIT_FAILURE);
    }

    // Loop continuously to receive requests and handle them
    while (1) {
        begin = clock();
        if (opt_flags.c_sleep > 0) {
            sleep(opt_flags.c_sleep);
        }
        socklen_t len = sizeof(clientAddr);
        res = recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr *) &clientAddr, &len);
        CHECK_RESULT(res, "recvfrom");
        handle_request(buffer, &clientAddr, len, &opt_flags, &env_var);
    }
    return 0;
}

void help() {
    printf("Usage: %s [OPTION]...\n", program_name);
    printf("Options:\n");
    printf(" -h, display this help and exit\n");
    printf(" -v, output version information and exit\n");
    printf(" -a, set custom IP address\n");
    printf(" -w, set sleep time \n");
    printf(" -d, run the program in daemon mode \n");
    printf(" -l, path to log file \n");
    printf(" -p , set custom port address \n");
}

void version() {
    printf("%s version 1.0\n", program_name);
    printf("info: Server return real number to client, which is the area under the curve line given by the coordinates on the plane\n");
    printf("Author: Karimov Eraj\n");
    printf("Group: N32501\n");
    printf("Lab: 2 variant 31\n");
}

bool check_arg(int argc, char **argv, struct opt_vars *opt_flags, struct env_vars *env_var) {
    int option = 0;
    while ((option = getopt(argc, argv, "w:hvl:da:p:")) != -1) {
        switch (option) {
            case 'w':
                if (optarg != NULL) {
                    opt_flags->c_sleep = atoi(optarg);
                    opt_flags->flag_wait = true;
                    if (env_var->env_debug) {
                        printf("SLEEPING TIME: %d\n", opt_flags->c_sleep);
                    }
                }
                break;
            case 'l':
                if (optarg != NULL) {
                    opt_flags->path_to_log = optarg;
                    opt_flags->flag_path_to_log = true;
                    if (env_var->env_debug) {
                        printf("LOG: %s\n", opt_flags->path_to_log);
                    }
                }
                break;
            case 'a':
                if (optarg != NULL) {
                    opt_flags->ip = optarg;
                    opt_flags->flag_ip = true;
                    if (env_var->env_debug) {
                        printf("IP: %s\n", opt_flags->ip);
                    }
                }
                break;
            case 'p':
                if (optarg != NULL) {
                    opt_flags->port = atoi(optarg);
                    opt_flags->flag_port = true;
                    if (env_var->env_debug) {
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
            case 'd':
                turn_on_daemon();
                break;
            case '?':
                printf("Incorrect argument, use --help");
                return false;
        }
    }
    return true;
}

void check_env(struct opt_vars *opt_flags, struct env_vars *env_var) {

    if (env_var->env_ip && !opt_flags->flag_ip) {
        opt_flags->ip = env_var->env_ip;
        if (env_var->env_debug) {
            printf("IP : %s\n", opt_flags->ip);
        }
    }
    if (env_var->env_port && !opt_flags->flag_port) {
        opt_flags->port = atoi(env_var->env_port);
        if (env_var->env_debug) {
            printf("PORT: %d\n", opt_flags->port);
        }
    }
    if (env_var->env_log && !opt_flags->flag_path_to_log) {
        opt_flags->path_to_log = env_var->env_log;
        if (env_var->env_debug) {
            printf("LOG : %s\n", opt_flags->path_to_log);
        }
    }
    if (env_var->env_wait && !opt_flags->flag_wait) {
        opt_flags->c_sleep = atoi(env_var->env_wait);
        if (env_var->env_debug) {
            printf("Wait time : %d\n", opt_flags->c_sleep);
        }
    }
}


double calculate_area(double x1, double y1, double x2, double y2) {
    return (y1 + y2) * (x2 - x1) / 2.0;
}

double found_area(char *buffer) {
    char *tok = strtok(buffer, " \t\n");
    double prev_x = strtod(tok, NULL);
    double prev_y = strtod(strtok(NULL, " \t\n"), NULL);
    double area = 0.0;
    printf("x1 = %f y1 = %f \n", prev_x, prev_y);
    while ((tok = strtok(NULL, " \t\n")) != NULL) {
        double x = strtod(tok, NULL);
        double y = strtod(strtok(NULL, " \t\n"), NULL);
        printf("x2 = %f y2 = %f \n", x, y);
        area += calculate_area(prev_x, prev_y, x, y);
        printf("\n area = %f \n", area);
        prev_x = x;
        prev_y = y;
    }
    return area;
}


void handle_request(char *buffer, struct sockaddr_in *clientAddr, socklen_t len, struct opt_vars *opt_flags,
                    struct env_vars *env_var) {
    ++users;
    int res;
    if (getenv("LAB2DEBUG")) {
        printf("Data received (%d bytes): [%s] from %s:%d\n", (int) strlen(buffer), buffer,
               inet_ntoa(clientAddr->sin_addr), ntohs(clientAddr->sin_port));
    }
    int pid = fork();
    struct tm *get_current_time;
    if (pid == 0) {
    	printf("Connected\n");
        FILE *file = fopen(opt_flags->path_to_log, "w");
        fprintf(file, "NEW CLIENT CONNECTED\n");
        char response[RESPONCE_SIZE];
        snprintf(response, RESPONCE_SIZE, "%.2f\n", found_area(buffer));
        res = sendto(serverSocket, response, RESPONCE_SIZE, 0, (const struct sockaddr *) clientAddr, len);
        if (res < 0) {
            perror("sendto");
        }
        end = clock();
        cpu_time = ((double) (end - begin)) / CLOCKS_PER_SEC;
        get_current_time = localtime(&current_time);
        strftime(save_time_begin, sizeof(save_time_begin), "%d.%m.%y %H:%M:%S", get_current_time);
        if (env_var->env_debug) {
            fprintf(stderr,
                    "The server has worked %lf of the time. Processed %d requests without errors and %d requests with errors\n",
                    cpu_time, users, error_req);
        } else {
            fprintf(file,
                    "%s :The server has worked %lf of the time. Processed %d requests without errors and %d requests with errors\n",
                    save_time_begin, cpu_time, users, error_req);
        }
        fclose(file);
        exit(EXIT_SUCCESS);
    } else if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

void signal_handler(int __attribute__((unused))(signum)) {
    printf("\nSee you!!! :) :) :)\t");
    printf("Exiting ...\n");
    exit(EXIT_SUCCESS);
}

void user_sig_handler(int __attribute__((unused))(signum)) {
    fprintf(stderr,
            "The server has worked %lf of the time. Processed %d requests without errors and %d requests with errors\n",
            cpu_time, users, error_req);
    exit(EXIT_SUCCESS);
}

bool check_signal(struct sigaction *signal_state) {
    if (sigaction(SIGUSR1, signal_state, NULL) == -1) {
        perror("sigaction(SIGUSR1) failed");
        return true;
    }
    if (sigaction(SIGINT, signal_state, NULL) == -1) {
        perror("sigaction(SIGINT) failed");
        return true;
    }
    if (sigaction(SIGTERM, signal_state, NULL) == -1) {
        perror("sigaction(SIGTERM) failed");
        return true;
    }
    if (sigaction(SIGQUIT, signal_state, NULL) == -1) {
        perror("sigaction(SIGQUIT) failed");
        return true;
    }
    return false;
}

void turn_on_daemon() {
    struct sigaction signal_state;
    signal_state.sa_handler = SIG_IGN;
    sigemptyset(&signal_state.sa_mask);
    signal_state.sa_flags = 0;

    if (sigaction(SIGTTOU, &signal_state, NULL) == -1) {
        perror("sigaction(SIGTTOU) failed");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGTSTP, &signal_state, NULL) == -1) {
        perror("sigaction(SIGTSTP) failed");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGCHLD, &signal_state, NULL) == -1) {
        perror("sigaction(SIGCHLD) failed");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGTTIN, &signal_state, NULL) == -1) {
        perror("sigaction(SIGTTIN) failed");
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed in daemon");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    if (setsid() < 0) {
        perror("Setsid failed in daemon");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGTERM, &signal_state, NULL) == -1) {
        perror("sigaction(SIGTERM) failed");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGHUP, &signal_state, NULL) == -1) {
        perror("sigaction(SIGHUP) failed");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGINT, &signal_state, NULL) == -1) {
        perror("sigaction(SIGINT) failed");
        exit(EXIT_FAILURE);
    }
    if (chdir("/") == -1) {
        perror("Chdir failed in daemon");
        exit(EXIT_FAILURE);
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}
