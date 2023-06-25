#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ftw.h>
#include "plugin_api.h"

static char* g_lib_name = "libkebN32501.so";
static char* g_plugin_purpose = "Search for files containing a double-precision real number in binary form";
static char* g_plugin_author = "Karimov Eraj Bahromjonovich";
char big_endian[65] = {0};
char little_endian[65] = {0};

#define DOUBLE_BIN_STR "double-bin"		//определяю название длинной опции

static struct plugin_option g_po_arr[] = {
    /*
        struct plugin_option {				// определяю структуру, содержащую мою длинную опцию 
            struct option {
               const char *name;
               int         has_arg;
               int        *flag;
               int         val;
            } opt,
            char *opt_descr
        }
    */
        {
            {
                DOUBLE_BIN_STR,
                required_argument,
                0, 0,
            },
            "Target double number in file"
        },
};

static int g_po_arr_len = sizeof(g_po_arr) / sizeof(g_po_arr[0]);


void convert_bin(double ); 
int plugin_get_info(struct plugin_info* ppi) 
 {
    	if (!ppi) {
        	fprintf(stderr, "ERROR: invalid argument\n");
        	return -1;
        }
    ppi->plugin_purpose = g_plugin_purpose;
    ppi->plugin_author = g_plugin_author;
    ppi->sup_opts_len = g_po_arr_len;
    ppi->sup_opts = g_po_arr;

    return 0;
 }

int plugin_process_file(const char* fname, struct option in_opts[], size_t in_opts_len) 
{

    
    int ret = -1;   		//возвращаем ошибку по умолчанию
    char* DEBUG = getenv("LAB1DEBUG");

    if (!fname || !in_opts || !in_opts_len) {
        errno = EINVAL;
        return -1;
    }

    if (DEBUG) {
        for (size_t i = 0; i < in_opts_len; i++) {				//Выводим информацию по полученной опции
            fprintf(stderr, "DEBUG: %s: Got option '%s' with arg '%s'\n",
                g_lib_name, in_opts[i].name, (char*)in_opts[i].flag);
        }
    }

    double inputnumber = 0.0;
    int got_inputnumber = 0;

#define OPT_CHECK(opt_var, is_double) \
    if (got_##opt_var) { \
        if (DEBUG) { \
            fprintf(stderr, "DEBUG: %s: Option '%s' was already supplied\n", \
                g_lib_name, in_opts[i].name); \
        } \
        errno = EINVAL; \
        return -1; \
    } \
    else { \
        char *endptr = NULL; \
        opt_var = is_double ? \
                    strtod((char*)in_opts[i].flag, &endptr): \
                    strtol((char*)in_opts[i].flag, &endptr, 0); \
        if (*endptr != '\0') { \
            if (DEBUG) { \
                fprintf(stderr, "DEBUG: %s: Failed to convert '%s'\n", \
                    g_lib_name, (char*)in_opts[i].flag); \
            } \
            errno = EINVAL; \
            return -1; \
        } \
        got_##opt_var = 1; \
    }

    for (size_t i = 0; i < in_opts_len; i++) {
        if (!strcmp(in_opts[i].name, DOUBLE_BIN_STR)) {
            OPT_CHECK(inputnumber, 1);
        }

        else {
            errno = EINVAL;
            return -1;
        }
    }

    if (!got_inputnumber) {
        if (DEBUG) {
            fprintf(stderr, "DEBUG: %s: Double value was not supplied.\n",
                g_lib_name);
        }
        errno = EINVAL;
        return -1;
    }

    

    if (DEBUG) {
        fprintf(stderr, "DEBUG: %s: Input: double value = %lf\n",
            g_lib_name, inputnumber);
    }

    int saved_errno = 0;

    FILE* fp = fopen(fname, "r");
    int fd = open(fname, O_RDONLY);
    if (fd < 0) {
        // errno is set by open()
        return -1;
    }

    struct stat st = {0};
    int res = fstat(fd, &st);
    if (res < 0) {
        saved_errno = errno;
        goto END;
    }

    
    if (st.st_size == 0) {			//проверяем, что файл ненулевой, так как нам нужны файлы с бинарным числом
        if (DEBUG) {
            fprintf(stderr, "DEBUG: %s: File size should be > 0\n",
                g_lib_name);
        }
        saved_errno = ERANGE;
        goto END;
    }

    convert_bin(inputnumber);
    for(int i=0; i<32; i+=8){					//из big-endian переводим в little-endian
    	int j=i;
    	for(int k=0;k<8;k++){
    	char a=little_endian[j+k];
    	little_endian[j+k]=little_endian[56+k-j];
    	little_endian[56+k-j]=a;
    	}
    
    }

    char *currentline=NULL;
    size_t len = 0;
    ssize_t read;
   
    while ((read = getline(&currentline, &len, fp)) != (ssize_t)-1)
    {
        if (strstr(currentline, big_endian) || strstr(currentline, little_endian))
        {     
        	if (DEBUG){
           fprintf(stderr, "DEBUG: %s: number found in file: %s\n", g_lib_name, fname);
        	}
        	if (DEBUG){
        fprintf(stderr, "DEBUG: %s: Converted double values:\nlittle-endian: %s\nbig endian: %s\n",
            g_lib_name, little_endian, big_endian);
    		}
                if (currentline) free(currentline);
                ret=0;
                goto END;
        }
       
        ret=1;
    }
   if ((currentline)&&(ret)) free(currentline);
    


END:
    fclose(fp);
    close(fd);


    //Restore errno value
    
    errno = saved_errno;
    return ret;
}

void convert_bin(double value){   //Функция, переводящая value в бинарный вид, заполняющая массивы little_endian и big_endian
    unsigned char* s = (unsigned char*)&value;
    int count = 0;

    for (int i = 0; i < 8; i++){
        for (int j = 0; j < 8; j++){
            if (*s & 1){
                little_endian[63-count] = '1';
                big_endian[63-count] = '1';
            }
            else{
                little_endian[63-count] = '0';
                big_endian[63-count] = '0';
            }
            *s >>= 1;
            count++;
        }
        s++;
    }
}


