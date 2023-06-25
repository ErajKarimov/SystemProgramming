// Search bytes in a file using regular expression
//
// (c) Eraj Karimov, 2023
// This source is licensed under CC BY-NC 4.0
// (https://creativecommons.org/licenses/by-nc/4.0/)
//

#include <regex.h> 
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  

int search_function(const char* filename, const char* bytes) {
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("File opening error");
    return -1;
  }

  // Set file pointer to end of file to get its size
  fseek(fp, 0, SEEK_END);
  long filesize = ftell(fp);
  // Return file pointer to beginning of file
  fseek(fp, 0, SEEK_SET);
  
  char* filebuf = malloc(filesize);
  
  if (filebuf == NULL) {
    perror("Memory allocation error");
    fclose(fp);
    return -1;
  }

  // Read file contents into buffer
  long bytes_read = fread(filebuf, sizeof(char), filesize, fp);
  if (bytes_read != filesize) {
    perror("File reading error");
    free(filebuf);
    fclose(fp);
    return -1;
  }

  // Compile regular expression to search for specified bytes
  regex_t regex;
  int ret = regcomp(&regex,bytes, 0);
  if (ret != 0) {
    perror("Regular expression compilation error");
    free(filebuf);
    fclose(fp);
    regfree(&regex);
    return -1;
  }

  // Variable to store number of matches found
  int matchcount = 0;
  // Variable to store information about the match found
  regmatch_t match = {0};
  // Pointer to current position in buffer
  char* p = filebuf;
  // Variable to store value of environment variable LAB11DEBUG
  char* result = getenv("LAB11DEBUG");

  // While regular expression finds matches in buffer
  while (regexec(&regex, p, 1, &match, 0) == 0) {
    matchcount++;
    // Move pointer to next position after the match found
    p += match.rm_eo;
    if (result != NULL) {
      fprintf(stderr, "The beginning of the match(pos): %ld\n",
              p - filebuf - strlen(bytes));
      fprintf(stderr, "The end of the match(pos): %ld\n", p - filebuf);
    }
  }
  // Free resources, return and return number of matches found
  regfree(&regex);
  free(filebuf);
  fclose(fp);
  return matchcount;
}
