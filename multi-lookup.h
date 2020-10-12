#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

// Used for INET6_ADDRSTRLEN
#include<arpa/inet.h>

#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MAX_REQUESTER_THREADS 5
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

#define DEBUG_PRINT 1
#define DEBUG_PRINT_THREAD 0
#define DEBUG_PRINT_NEXT_FILE 0
#define ARR_SIZE 10

typedef struct req_params_s{
	FILE * log_file;
	void * buffer;
	char ** argv;
	int * index;
	int * count;
	int * out;
	pthread_mutex_t * mutex_index;
	int * argc;
} req_params_t;

typedef struct res_params_s{
	FILE * log_file;
	void * buffer;
	int * count;
	int * in;
} res_params_t;

void * req_func(void * ptr);
void * res_func(void * ptr);
int create_pool(int num, pthread_t * arr, void * func, void * params);
int join_pool(int num, pthread_t * arr);
int open_log(FILE ** log, char * file_name);
int check_args(int argc, char * argv[]);
int get_req_res_num(long * req_num, long * res_num, char * argv[]);
int create_req_params(req_params_t * res_params, char ** argv, FILE * log_file, void * buffer, int * index, int * count, int * out, pthread_mutex_t * mutex_index, int * argc);
int create_res_params(res_params_t * res_params, FILE * log_file, void * buffer, int * count, int * in);
int get_next_file(FILE ** input_file, pthread_mutex_t * mutex_index, int * argc, int * index, char ** argv);

#endif




