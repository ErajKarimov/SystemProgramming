//
// Search bytes.
//
// (c) Eraj Karimov, 2023
// This source is licensed under CC BY-NC 4.0
// (https://creativecommons.org/licenses/by-nc/4.0/)
//

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h> // for MIN()
#include <sys/types.h>

// Define a constant for maximum indentation level in output
#define MAX_INDENT_LEVEL 128

// Declare global variables for program name and bytes to search for
const char* program_name;
char* bytes;

void walk_dir(char* dir);

void print_help(void);

void print_version(void);

int search_function(const char* filename, const char* bytes);

char* hex_to_bytes(const char* hexstr);

int main(int argc, char* argv[])
{
    int opt = 0;

    // Set program name from command line argument
    program_name = argv[0];

    // Define long options for getopt_long() function
    static struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 'v' },
        { 0, 0, 0, 0 }
    };

    // Parse command-line arguments using getopt_long() function
    while ((opt = getopt_long(argc, argv, "hv", long_options, NULL)) != -1) {
        switch (opt) {

        // If help flag is given, print usage/help message and exit
        case 'h':
            print_help();
            exit(EXIT_SUCCESS);

        // If version flag is given, print version information and exit
        case 'v':
            print_version();
            exit(EXIT_SUCCESS);

        // If an invalid or unrecognized option is given, print error message and exit
        default:
            printf("Try '%s --help' for more information.\n", program_name);
            exit(EXIT_FAILURE);
        }
    }

    // Check that the correct number of arguments is given
    if (argc != 3) {
        printf("Usage: %s <dir>\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Convert hexadecimal string to binary data and store in global variable 'bytes'
    bytes = hex_to_bytes(argv[2]);

    // Traverse directory and search for byte pattern in files
    walk_dir(argv[1]);

    // Free allocated memory for byte pattern data
    free(bytes);

    // Return success status
    return EXIT_SUCCESS;
}

// Function to convert a hexadecimal string to binary data
char* hex_to_bytes(const char* hexstr)
{

    size_t len = strlen(hexstr);

    // Check if input string is valid hexadecimal format
    if (len % 2 != 0 || len < 2 || hexstr[0] != '0' || hexstr[1] != 'x') {
        perror("Invalid hex string");
        exit(EXIT_FAILURE);
    }

    size_t hex_len = strlen(hexstr) - 2;
    char* hex_buf = malloc(hex_len / 2);

    if (hex_buf == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }

    // Convert hexadecimal to binary with scanf() function
    for (size_t i = 0; i < hex_len; i += 2) {
        sscanf(hexstr + 2 + i, "%2hhx", &hex_buf[i / 2]);
    }

    return hex_buf;
}

// Function to print help message
void print_help(void)
{
    printf("Usage: %s [OPTION]...\n", program_name);
    printf("Options:\n");
    printf("  -h, --help     display this help and exit\n");
    printf("  -v, --version  output version information and exit\n");
}

// Function to print version information
void print_version(void)
{
    printf("%s version 1.0\n", program_name);
    printf("info: Finds bytes in all files of the directory\n");
    printf("Author: Karimov Eraj\n");
    printf("Group: N32501\n");
    printf("Lab: 1.01\n");
}

// Function to print an entry for a file or directory in the directory structure
void print_entry(int level, int type, const char* path)
{
    if (!strcmp(path, ".") || !strcmp(path, ".."))
        return;

    char indent[MAX_INDENT_LEVEL] = { 0 };
    memset(indent, ' ', MIN((size_t)level, MAX_INDENT_LEVEL));
    printf("%s[%d] %s\n", indent, type, path);
    int found = search_function(path, bytes);
    printf("  matches found:%d\n", found);
}

// Function to recursively traverse a directory structure
void walk_dir_impl(int ind_level, char* dir)
{
    // Open directory for reading
    DIR* d = opendir(dir);
    if (d == NULL) {
        printf("Failed to opendir() %s\n", dir);
        return;
    }
    errno = 0;
    struct dirent* p = readdir(d);
    // Read all entries in directory and process each one
    while ((p = readdir(d)) != NULL) {
        if (strcmp(p->d_name, ".") && strcmp(p->d_name, "..")) {

            // Build path string for entry
            char buf2[PATH_MAX] = { 0 };
            sprintf(buf2, "%s/%s", dir, p->d_name);
            print_entry(ind_level + 1, p->d_type, buf2);

            // Recursively traverse subdirectory if entry is a directory
            if (p->d_type == DT_DIR) {
                char buf[PATH_MAX] = { 0 };
                sprintf(buf, "%s/%s", dir, p->d_name);
                walk_dir_impl(ind_level + 1, buf);
            }
        }
        errno = 0;
    }
    closedir(d);
}

// Function to traverse a directory and search for byte pattern in all files
void walk_dir(char* dir)
{
    print_entry(0, DT_DIR, dir);
    walk_dir_impl(0, dir);
}
