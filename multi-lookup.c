#include<pthread.h>
#include"util.h"
#include"multi-lookup.h"

int main(int argc, char * argv[]){

	// VARIABLES

	FILE * req_log = NULL;
	FILE * res_log = NULL;
	long req_num, res_num;
	// pthread_t * req_pool, res_pool; // DEFINED LATER
	void * buffer = malloc(MAX_NAME_LENGTH * ARR_SIZE);
	res_params_t res_params;
	req_params_t req_params;

	// CHECK COMMAND LINE ARGUMENTS
	if(check_args(argc, argv)) return -1;

	// CHECK FOR NUMBER OF REQUESTERS AND RESOLVERS
	if(get_res_req_num(&req_num, &res_num, argv)) return -1;

	// CHECK FOR LOG FILES
	if(open_log(&req_log, (char *) (argv[3])) != 0 || open_log(&res_log, (char *) (argv[4])) != 0) return -1;

	// CREATE ARGS FOR THREAD POOLS
	if(create_res_params(&res_params, argv, res_log, buffer) || create_req_params(&req_params, req_log, buffer)) return -1;

	// CREATE POOLS
	pthread_t req_pool[req_num];
	pthread_t res_pool[res_num];
	if( create_pool(req_num, (pthread_t *) (req_pool), req_func, (void *) (&req_params)) || 
		create_pool(res_num, (pthread_t * ) (res_pool), res_func, (void *) (&res_params))) return -1;

	// JOIN THREAD POOLS
	if(join_pool(req_num, (pthread_t *) (req_pool)) || join_pool(res_num, (pthread_t *) (res_pool))) return -1;

	// FREE MEMORY
	free(buffer);

}

void * res_func(void * ptr){
	// printf("res_func: Thread %lu\n", pthread_self());
	// Cast parameters
	res_params_t * res_params = (res_params_t *) ptr;
	printf("res_func: Thread %lu, FILE * %p, buffer * %p\n", pthread_self(), res_params->log_file, res_params->buffer);
	return 0;
}

void * req_func(void * ptr){
	// Cast parameters
	req_params_t * req_params = (req_params_t *) ptr;
	printf("req_func: Thread %lu, FILE * %p, buffer * %p\n", pthread_self(), req_params->log_file, req_params->buffer);
	return 0;
}

int create_res_params(res_params_t * res_params, char ** argv, FILE * log_file, void * buffer){
	res_params->argv = argv;
	res_params->log_file = log_file;
	res_params->buffer = buffer;
	return 0;
}

int create_req_params(req_params_t * req_params, FILE * log_file, void * buffer){
	req_params->log_file = log_file;
	req_params->buffer = buffer;
	return 0;
}

int get_res_req_num(long * res_num, long * req_num, char * argv[]){
	
	*(req_num) = strtol(argv[1], &argv[2]-1,10);
	if(*(req_num) < 0 || *(req_num) > MAX_REQUESTER_THREADS){
		printf("Ensure 1 to %d requesters\n", MAX_REQUESTER_THREADS);
		return -1;
	}
	if(DEBUG_PRINT) printf("Requesters: %ld\n", *(req_num));

	*(res_num) = strtol(argv[2], &argv[3]-1,10);
	if(*(res_num) < 0 || *(res_num) > MAX_RESOLVER_THREADS){
		printf("Ensure 1 to %d resolvers\n", MAX_RESOLVER_THREADS);
		return -1;
	}
	if(DEBUG_PRINT) printf("Resolvers: %ld\n", *(res_num));

	return 0;
}

int check_args(int argc, char * argv[]){
	// CHECK FOR ARGUMENT COUNT >= 5

	if(argc < 5){
		printf("Please enter the command as follows:\n");
		printf("multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ...]\n");
		return -1;
	}

	// CHECK LOG FILES ARE DIFFERENT

	if(strcmp(argv[3], argv[4]) == 0){
		printf("Error: req_log and res_log are the same\n");
		return -1;
	}

	return 0;
}

int create_pool(int num, pthread_t * arr, void * func, void * params){
	for(int i = 0; i < num; i++){
		if(pthread_create(&(arr[i]), NULL, func, params)){
			printf("create_pool: Failed to create thread %d\n", i);
			return -1;
		}
		if(DEBUG_PRINT) printf("create_pool: Created thread %d TID %lu\n", i, arr[i]);
	}
	return 0;
}

int join_pool(int num, pthread_t * arr){
	for(int i = 0; i < num; i++){
		if(pthread_join(arr[i], NULL)){
			printf("join_pool: Failed to join Thread %d TID %lu\n", i, arr[i]);
			return -1;
		}
		if(DEBUG_PRINT) printf("join_pool: Joined thread %d TID %lu\n", i, arr[i]);
	}
	return 0;
}

int open_log(FILE ** log, char * file_name){
	(*log) = fopen(file_name, "r+");
	if(log == NULL){
		printf("Error opening %s\n", file_name);
		return -1;
	}

	return 0;
}

