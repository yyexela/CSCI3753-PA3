#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

// Used for INET6_ADDRSTRLEN
#include<arpa/inet.h>

#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MAX_REQUESTER_THREADS 5
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

#define DEBUG_PRINT 0
#define DEBUG_PRINT_THREAD 0
#define DEBUG_PRINT_NEXT_FILE 0
#define DEBUG_PRINT_LOGS_STDOUT 0
#define DEBUG_PRINT_DNS 0
#define DEBUG_PRINT_REMOVE 0
#define DEBUG_PRINT_ADD 0
#define DEBUG_PRINT_COUNT 0
#define DEBUG_PRINT_MALLOC 0
#define DEBUG_PRINT_POOLS 0
#define DEBUG_PRINT_DONE 0
#define DEBUG_PRINT_LOGS 0
#define DEBUG_PRINT_PERF 0

#define ARR_SIZE 20
#define DISABLE_DNS 1

typedef struct req_params_s{
	FILE * log_file;
	void * buffer;
	char ** argv;
	int * index;
	int * count;
	int * in;
	int * argc;
	pthread_mutex_t * mutex_index;
	pthread_mutex_t * mutex_count;
	pthread_mutex_t * mutex_buffer;
	pthread_mutex_t * mutex_req_log;
	pthread_cond_t * cond_req;
	pthread_cond_t * cond_res;
	int * done;
} req_params_t;

typedef struct res_params_s{
	FILE * log_file;
	void * buffer;
	int * count;
	int * out;
	pthread_mutex_t * mutex_count;
	pthread_mutex_t * mutex_buffer;
	pthread_mutex_t * mutex_res_log;
	pthread_cond_t * cond_res;
	pthread_cond_t * cond_req;
	int * done;
} res_params_t;

void * req_func(void * ptr);
void * res_func(void * ptr);
int create_pool(int num, pthread_t * arr, void * func, void * params);
int join_pool(int num, pthread_t * arr);
int open_log(FILE ** log, char * file_name);
int check_args(int argc, char * argv[]);
int get_req_res_num(long * req_num, long * res_num, char * argv[]);
int get_next_file(FILE ** input_file, req_params_t * req_params);
int read_line(char * line, FILE * input_file);
void add_to_buffer(char * line, req_params_t * req_params);
int remove_from_buffer(char * line, res_params_t * res_params);
void add_to_req_log(char * line, req_params_t * req_params);
void add_to_res_log(char * name, char * ip_addr, res_params_t * res_params);
void add_to_req_log_final(int serviced, req_params_t * req_params);

#endif




