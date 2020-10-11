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
#define ARR_SIZE 10

void * req_func(void * ptr);
void * res_func(void * ptr);
int create_pool(int num, pthread_t * arr, void * func);
int join_pool(int num, pthread_t * arr);
int open_log(FILE * log, char * file_name);
int check_args(int argc, char * argv[]);
int get_res_req_num(long * res_num, long * req_num, char * argv[]);

#endif
