#define _GNU_SOURCE         // for get_current_dir_name()
#define _XOPEN_SOURCE 500   // for nftw()
#define UNUSED(x) (void)(x)
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ftw.h>
#include <getopt.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include "plugin_api.h"

int walk_func(const char*, const struct stat*, int, struct FTW*);  // функция, с помощью которой будем проверять каждый файл в указанном каталоге
void atexit_func(); // функция, которую вызываем в конце выполнения программы для чистки памяти
void print_info();	//выводим справку по опциям

const char *find_so_library(const char *filename) {   
    const char *so = strrchr(filename, '.');   				//функция, с помощью которой ищем последнюю точку в имени файла
    if(!so || so==filename) return "";   				//когда находим её, возвращаем указатель на то, что после нее
    return so + 1;							//таким образом ищем формат файла .so
}				

struct short_opt {     
    unsigned int NOT;
    unsigned int AND;				//структура, чтобы отслеживать, какие опции ввели, чтобы использовать для получения результата
    unsigned int OR;
};

struct libsinfo {
    struct plugin_option *opts; 		// массив опций плагина
    char *name;	                
    int opts_len;               
    void *dl;                   		// обработчик плагина
};

struct short_opt s_opt = {0, 0, 0};
struct libsinfo *all_libs; 			// массив с информацией о всех плагинах
struct option *long_opts; 			// массив всех длинных опций
char *pluginpath;  				// директория с плагинами
int libs_count = 0; 				// количество библиотек
int inputargs_count = 0; 				// количество длинных опций, введенных пользователем
char **input_args = NULL; 			// аргументы длинных опций, введенных пользователем
int *long_opt_index= NULL; 			// массив индексов длинных опций в массиве long_opts в порядке введения их пользователем
int *dl_index; 					// массив обработчиков плагинов
int isPset = 0; 				// флаг, обозначающий наличие опции -P

int main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Usage: %s <write directory>\nFor help enter flag -h\n", argv[0]);
        return 1;
    }
    char *search_dir = ""; 			// устанавливаем директорию под поиска как пустой символ, так как папки с таким именем быть не может
    pluginpath = get_current_dir_name(); 	// получаем текущую директорию под плагины по умолчанию
    atexit(&atexit_func); 			// устанавливаем функцию очистки выделенной памяти в конце программы


    for(int i=0; i < argc; i++) {
        if (strcmp(argv[i], "-P") == 0) {
            if (isPset) { 			// проверяем, вводили ли мы опцию
                fprintf(stderr, "ERROR: the option cannot be repeated -- 'P'\n");
                return 1;
            }
            if (i == (argc - 1)) { 		// проверяем, ввели ли мы что-то после опции -P
                fprintf(stderr, "ERROR: option 'P' needs an argument\n");
                return 1;
            }	
            DIR *d;				
            if ((d=opendir(argv[i+1])) == NULL) { 
                perror(argv[i+1]); 		// если директория под плагины пустая или не открылась, то возвращаем ошибку и завершаем
                return 1;
            } 
            else {
            	closedir(d);
            	free(pluginpath); 
                pluginpath = argv[i+1]; 	// в случае корректного ввода устанавливаем новый каталог под плагины			
                isPset = 1;
            }   
        }
        else if (strcmp(argv[i],"-v") == 0){  
            printf(" Лабораторная работа №1_2.\n Выполнил: Каримов Эрадж. \n Вариант: 15.\n");
                return 0;
        }
        else if (strcmp(argv[i],"-h") == 0) { 
            print_info();
            return 0;
        } 
    }
   
    DIR *d; 
    struct dirent *dir; 
    d = opendir(pluginpath);
    if (d != NULL) {
        while ((dir = readdir(d)) != NULL) { 
            if ((dir->d_type) == 8) { 			//открывем директорию с плагинами и проверяем, есть ли там файлы и считаем количество файлов, чтоб потом
                libs_count++;   			//не сделать ошибку и выделить достаточно памяти
            }
        }
        closedir(d);
    }
    else{
        perror("opendir"); 				
        exit(EXIT_FAILURE);
    }
    
    all_libs = (struct libsinfo*)malloc(libs_count*sizeof(struct libsinfo));	//делаем соответствующее выделение памяти
    if (all_libs==0){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    libs_count = 0; 					// счетчик в 0, чтобы начать заполнять нашу структуру с опциями
    d = opendir(pluginpath);
    if (d != NULL) {    
        while ((dir = readdir(d)) != NULL) {
            if ((dir->d_type) == 8) {
                if (strcmp(find_so_library(dir->d_name),"so")==0){ 		// ищем соотвествующие динамические библиотеки с расширением .so
                    char filename[258]; 					// создаем массив, чтобы туда записывать путь к файлу
                    snprintf(filename, sizeof filename, "%s/%s", pluginpath, dir->d_name);  //заполняем filename в виду Путь к плагину/название библиотеки
										       
                    void *dl = dlopen(filename, RTLD_LAZY); 			// открываем плагин и получаем указатель на начало динамической библиотеки
                    if (!dl) {
                        fprintf(stderr, "ERROR: dlopen() failed: %s\n", dlerror());
                        continue;
                    }
                    void *func = dlsym(dl, "plugin_get_info"); 			// читаем функцию из плагина, получаем указатель на место plugin_get_info
                    if (!func) {
                        fprintf(stderr, "ERROR: dlsym() failed: %s\n", dlerror());
                    }
                    struct plugin_info pi = {0}; 				// создаем структуру для получения информации из плагина (ее передаем в функции)
                    typedef int (*pgi_func_t)(struct plugin_info*);
                    pgi_func_t pgi_func = (pgi_func_t)func;	

                    int ret = pgi_func(&pi); 					// читаем информацию из плагина
                    if (ret < 0) {
                        fprintf(stderr, "ERROR: plugin_get_info() failed '%s'\n", dir->d_name);
                    }
                    all_libs[libs_count].name = dir->d_name;       
                    all_libs[libs_count].opts = pi.sup_opts;      			//заполняем структуру, содержащую длинную опцию
                    all_libs[libs_count].opts_len = pi.sup_opts_len;
                    all_libs[libs_count].dl = dl;
                    if (getenv("LAB1DEBUG")) { 
       		    	    printf("DEBUG: Library found:\n\tPlugin name: %s\n\tPlugin purpose: %s\n\tPlugin author: %s\n", dir->d_name, pi.plugin_purpose, pi.plugin_author);
                    }
                    libs_count++;
                }
            }
        }
        closedir(d);
    }


    
    size_t opt_count = 0; // отвечает за суммарное количество всех опций
    for(int i = 0; i < libs_count; i++) { 
        opt_count += all_libs[i].opts_len;
    }

    long_opts=(struct option*)malloc(opt_count*sizeof(struct option));		//выделяем память под массив структур типа option
    if (!long_opts){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    opt_count = 0; 
    for(int i = 0; i < libs_count; i++) {
        for(int j = 0; j < all_libs[i].opts_len; j++) {				//заполняем массив опциями
            long_opts[opt_count] = all_libs[i].opts[j].opt;
            opt_count++; 
        }
    }

    int flag;
    for(int i=0; i < argc; i++) {
        if (strstr(argv[i], "--")) { // ищем нашу длинную опцию и затем сравниваем её с именем из нашего массива структур
            flag = 0;
            for(size_t j=0; j<opt_count; j++) {
                if (strcmp(&argv[i][2], long_opts[j].name) == 0) {
                    flag = 1; 
                }
            }
            // если не нашли, то даем ошибку
            if (flag == 0) {
                fprintf(stderr, "ERROR: option '%s' is not found\n", argv[i]);
                return 1;
            }
        }
    }

    // используаем getopt для простой обработки входных данных
    int c; // в этой переменной getopt возвращает короткую опцию, если нашел таковую
    int is_sdir_set = 0; // отвечает за то, ввели ли мы директорию под поиск
    int optindex = -1; 
    while((c = getopt_long(argc, argv, "-P:AON", long_opts, &optindex))!=-1) {			// long_opts наш массив с длинными опциями, если находит, то возвращает её индекс в optindex
        switch(c) {										// ключ "-" отвечает за то, чтобы каждый аргумент строки считать как аргумент к опции 
            case 'O':
                if (!s_opt.OR) {								//проверяем, была ли введена опция O
                    if (!s_opt.AND) {					
                        s_opt.OR = 1;
                    } 
                    else { 									// если введены и A и O, то это ошибка
                        fprintf(stderr, "ERROR: can be either 'A' or 'O' \n");
                        return 1;
                    } 
                } 
                else {
                    fprintf(stderr, "ERROR: the option 'O' can't be repeated\n");
                    return 1;
                }
                break;
            case 'A':
                if (!s_opt.AND) {
                    if (!s_opt.OR) {
                        s_opt.AND = 1;
                    } 
                    else {
                        fprintf(stderr, "ERROR: can be either 'A' or 'O' \n");
                        return 1;
                    }
                } 
                else {
                    fprintf(stderr, "ERROR: the option 'A' cannot be repeated\n");
                    return 1;
                }
                break;
            case 'N':
                if (!s_opt.NOT){
                    s_opt.NOT = 1;// данные проверки для предотвращения повторов опции N
                }
                else{
                    fprintf(stderr, "ERROR: the option cannot be repeated -- 'N'\n");
                    return 1;
                }
                break;
            case 'P':
            	break;
            case ':':							 // выходим, если у короткой опции пропущен аргумент
                return 1;
            case '?':							 // выходим, если ввели короткую опцию неправильно
                return 1;
            default: 							
                if(optindex != -1) { 					 // заходим, если нашли длинную опцию
                    inputargs_count++; 					 // увеличиваем счетчик длинных опций
                    if (getenv("LAB1DEBUG")) {
                        printf("DEBUG: Found option '%s' with argument: '%s'\n", long_opts[optindex].name, optarg);
                    }
                    long_opt_index = (int*) realloc (long_opt_index, inputargs_count * sizeof(int));  // выделяем память, чтобы записывать сюда индексы длинных опций
                    if (!long_opt_index){
                        perror("realloc");
                        exit(EXIT_FAILURE);
                    }
                    input_args = (char **) realloc (input_args, inputargs_count * sizeof(char *));
                    if (!input_args){
                        perror("realloc");
                        exit(EXIT_FAILURE);
                    }		 

                    input_args[inputargs_count - 1] = optarg; 		  // записываем аргумент опции 
                    long_opt_index[inputargs_count - 1] = optindex;       // записываем индекс нашей длинной опции
                    optindex = -1;
                } 
                else { 							  //значит это не длинная опция
                    if (is_sdir_set) { 				          // проверяем, вводили ли мы директорию под поиск
                        fprintf(stderr, "ERROR: the examine directory has already been set at '%s'\n", search_dir );
                        return 1;
                    }
                    if ((d = opendir(optarg))== NULL) {    		   //проверяем директорию под поиск
                        perror(optarg);					   //т.к. стоит ключ "-", то элемент строки, не являющийся длинной опцией, будет являться аргументом и записывать в optarg
                        return 1;
                    } 
                    else {
                        search_dir  = optarg;  				   //устанавливаем директорию под поиск
                        is_sdir_set = 1;
                        closedir(d);
                    }
                }
        } 
    }

    
    if (strcmp(search_dir , "") == 0) {					    //проверяем, ввели ли мы директорию
        fprintf(stderr, "ERROR: The directory for research isn't specified\n");
        return 1;
    }
    if (getenv("LAB1DEBUG")) {
        printf("DEBUG: Directory for research: %s\n", search_dir );
    }

    
    if ((s_opt.AND==0) && (s_opt.OR==0) ) { 				    //по умолчанию флаг AND=1
        s_opt.AND = 1;		
    }
    
    if (getenv("LAB1DEBUG")) {
        printf("DEBUG: Information about input comands:\n\tAND: %d\tOR: %d\tNOT: %d\n", s_opt.AND, s_opt.OR, s_opt.NOT);
    }
    
    dl_index = (int *)malloc(inputargs_count*sizeof(int));
    if (!dl_index){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    
    for(int i=0; i < inputargs_count; i++) {				//Ищем соответствие наших введенных опций в общем массиве опций		
        const char *opt_name = long_opts[long_opt_index[i]].name; 	// записываем имя опции, чтобы по нему сопоставлять опцию и её индекс
        for(int j=0; j < libs_count; j++) { 
            for(int k=0; k < all_libs[j].opts_len; k++) { 		// проходимся по всем опциям каждой библиотеки и сравниваем с именем нашей опции
                if (strcmp(opt_name, all_libs[j].opts[k].opt.name) == 0) { // ищем индекс библиотеки в массиве all_libs для текущей опции opt_name  				
                    dl_index[i] = j; 					// сохраняем индекс найденной опции в массив индексов
                }
            }
        }
    }
	
    if (getenv("LAB1DEBUG")) {
        printf("DEBUG: Directory with libraries: %s\n", pluginpath);
    }
    
    int res = nftw(search_dir , walk_func, 20, FTW_PHYS || FTW_DEPTH);	  //с помощью nftw применяем к каждому элемента дерева начиная с search_dir нашу walk_func
    if (res < 0) {
        perror("nftw");
        return 1;
    }
    return 0;

}
   
typedef int (*proc_func_t)( const char *name, struct option in_opts[], size_t in_opts_len);
int walk_func(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_F) {   						// если файл - обычный файл, то обрабатываем его
        int rescondition = s_opt.NOT ^ s_opt.AND; 			// устанавливаем начальное значение в зависимости от флагов A O N
        for (int i = 0; i < inputargs_count; i++) { 			// проверяем файл по всем введенным нами опциям
            struct option opt = long_opts[long_opt_index[i]]; 		// получаем текущую опцию с помощью индекса, который искали
            char * arg = input_args[i];
            if (arg) { 							// определяем ее аргумент, если он есть(должен быть)
                opt.has_arg = 1;
                opt.flag = (void *)arg;					
            } else {
                opt.has_arg = 0;
            }

    	    void *func = dlsym(all_libs[dl_index[i]].dl, "plugin_process_file");    //т.к. мы получали dl заранее и сохраняли его в массив, то зная индекс можно его спокойно найти, и использовать
            proc_func_t proc_func = (proc_func_t)func; 				    // приводим указатель на функцию к нужному типу
            int res_func;
            res_func = proc_func(fpath, &opt, 1); 				    // проверяем файл
            
            if (res_func) {							    
                if (res_func > 0) {
                    res_func = 0;
                } 
                else {
                    fprintf(stderr, "Plugin execution error\n");
                    return 1;
                }
            } 
            else {
                res_func = 1;
            }


            if (s_opt.NOT ^ s_opt.AND) {
                rescondition = rescondition & (s_opt.NOT ^ res_func);
            } else {
                rescondition = rescondition | (s_opt.NOT ^ res_func);
            }
        }

        if (rescondition) {
            printf("\tSuitable file: %s\n",fpath);			//Выводим подходящий файл
        }
        else{
            if (getenv("LAB1DEBUG")) {
                printf("\tDEBUG: Unsuitable file: %s \n", fpath);	//Если файл не подходит под критерий, то если задана переменная окружения, выводим неподходящий файл
            }
        }
    }	
    		
    UNUSED(sb);  
    UNUSED(ftwbuf);							//неиспользуемые переменные считаются ошибками
    return 0;
}



void atexit_func() {
    for (int i=0;i<libs_count;i++){
        if (all_libs[i].dl) dlclose(all_libs[i].dl);
    }
    if (all_libs) free(all_libs);					//используем для того, чтобы очищать то, под что выделили память
    if (dl_index) free(dl_index);
    if (long_opts) free(long_opts);
    if (long_opt_index) free(long_opt_index);
    if (input_args) free(input_args);
    if (!isPset) free(pluginpath); 
}


void print_info(){
	 printf("-P <dir>  Задать каталог с плагинами.\n");
         printf("-A        Объединение опций плагинов с помощью операции «И» (действует по умолчанию).\n");
         printf("-O        Объединение опций плагинов с помощью операции «ИЛИ».\n");
         printf("-N        Инвертирование условия поиска (после объединения опций плагинов с помощью -A или -O).\n");
         printf("-v        Вывод версии программы и информации о программе (ФИО исполнителя, номер группы, номер варианта лабораторной).\n");
         printf("-h        Вывод справки по опциям.\n");
}

