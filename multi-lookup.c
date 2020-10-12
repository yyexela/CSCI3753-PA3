#include<pthread.h>
#include <unistd.h> // TODO: Remove when done, for sleep(n)
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
	int out = 0; // producer
	int in = 0; // consumer
	int count = 0; // # in buffer
	int index = 5; // position of next file input
	int done = 0;

	// MUTEXES
	// TODO Wrap in case of error

	pthread_mutex_t mutex_index;
	pthread_mutex_t mutex_count;
	pthread_mutex_t mutex_buffer;
	pthread_mutex_t mutex_done;
	pthread_mutex_init(&mutex_index, NULL);
	pthread_mutex_init(&mutex_count, NULL);
	pthread_mutex_init(&mutex_buffer, NULL);
	pthread_mutex_init(&mutex_done, NULL);

	// CONDITIONAL VARIABLES
	// TODO Wrap in case of error

	pthread_cond_t cond_req;
	pthread_cond_t cond_res;
	pthread_cond_init(&cond_req, NULL);
	pthread_cond_init(&cond_res, NULL);

	// CHECK COMMAND LINE ARGUMENTS
	if(check_args(argc, argv)) return -1;

	// CHECK FOR NUMBER OF REQUESTERS AND RESOLVERS
	if(get_req_res_num(&req_num, &res_num, argv)) return -1;

	// CHECK FOR LOG FILES
	if(open_log(&req_log, (char *) (argv[3])) != 0 || open_log(&res_log, (char *) (argv[4])) != 0) return -1;

	// CREATE ARGS FOR THREAD POOLS
	if(create_req_params(&req_params, argv, req_log, buffer, &index, &count, &in, &argc, &mutex_index, &mutex_count, &mutex_buffer, &mutex_done, &cond_req, &cond_res, &done) || create_res_params(&res_params, res_log, buffer, &count, &out, &mutex_count, &mutex_buffer, &mutex_done, &cond_req, &cond_res, &done)) return 0;

	// CREATE POOLS
	pthread_t req_pool[req_num];
	pthread_t res_pool[res_num];
	if( create_pool(req_num, (pthread_t *) (req_pool), req_func, (void *) (&req_params)) || 
		create_pool(res_num, (pthread_t * ) (res_pool), res_func, (void *) (&res_params))) return -1;

	// JOIN PRODUCER THREAD POOL
	if(join_pool(req_num, (pthread_t *) (req_pool))) return -1;

	// INFORM CONSUMERS TO EXIT
	pthread_mutex_lock(&mutex_done);
		done = 1;
		pthread_cond_broadcast(&cond_res);
	pthread_mutex_unlock(&mutex_done);

	// JOIN CONSUMER THREAD POOL
	if(join_pool(res_num, (pthread_t *) (res_pool))) return -1;

	// FREE MEMORY
	free(buffer);

	if(DEBUG_PRINT) printf("Successfully finished\n");

	return 0;
}

void * req_func(void * ptr){
	// Cast parameters to struct
	req_params_t * req_params = (req_params_t *) ptr;

	// Create thread-specific variables
	FILE * input_file = NULL;

	// Main producer loop
	while(1){
		// Get a file
		if(get_next_file(&input_file, req_params)) return 0;
		
		// Produce lines from the file into the buffer
		read_line(input_file, req_params);
	}	
	return 0;
}

void * res_func(void * ptr){
	// Cast parameters to struct
	res_params_t * res_params = (res_params_t *) ptr;

	// Main consumer loop
	while(remove_from_buffer(res_params)){
	}

	// Create thread-specific variables
	return 0;
}

// return 0 only when done, return 1 when read item
int remove_from_buffer(res_params_t * res_params){
	char line[MAX_NAME_LENGTH];
	pthread_mutex_lock(res_params->mutex_count);
		while(*(res_params->count) <= 0){
			pthread_mutex_lock(res_params->mutex_done);
				if(*(res_params->done)){
					pthread_mutex_unlock(res_params->mutex_done);
					pthread_mutex_unlock(res_params->mutex_count);
					return 0;
				}
			pthread_mutex_unlock(res_params->mutex_done);
			pthread_cond_wait(res_params->cond_res, res_params->mutex_count);
		}
		pthread_mutex_lock(res_params->mutex_buffer);
			strcpy(line, res_params->buffer + (MAX_NAME_LENGTH * (*(res_params->out))));
			*(res_params->out) = (*(res_params->out)) + 1 % ARR_SIZE;
			*(res_params->count) = (*(res_params->count)) - 1;
			if(DEBUG_PRINT && DEBUG_PRINT_REMOVE && DEBUG_PRINT_COUNT) printf("Count: %d\n", *(res_params->count));
		pthread_mutex_unlock(res_params->mutex_buffer);
	pthread_mutex_unlock(res_params->mutex_count);
	pthread_cond_signal(res_params->cond_req);

	if(DEBUG_PRINT && DEBUG_PRINT_REMOVE) printf("Consumer read %s\n", line);
	return 1;
}

void read_line(FILE * input_file, req_params_t * req_params){
	char line[MAX_NAME_LENGTH];
	//char ip_addr[MAX_IP_LENGTH];
	while(fgets(line, MAX_NAME_LENGTH, input_file) != NULL){
		// TODO: See if newline is ok to remove
		long bytes = strlen(line);
		if(bytes != 0){
			line[bytes-1] = '\0';
		}
		add_to_buffer((char *) line, req_params);
		/*if(dnslookup(line, ip_addr, MAX_IP_LENGTH)){
			if(DEBUG_PRINT && DEBUG_PRINT_DNS) printf("DNS for %s failed\n", line);
			ip_addr[0] = '\0';
		}
		if(DEBUG_PRINT && DEBUG_PRINT_DNS) printf("Thread %lu reads %s as %s\n", pthread_self(), line, ip_addr);
		*/
	}
}

void add_to_buffer(char * line, req_params_t * req_params){
	pthread_mutex_lock(req_params->mutex_count);
		while(*(req_params->count) >= ARR_SIZE-1){
			pthread_cond_wait(req_params->cond_req, req_params->mutex_count);
		}
		pthread_mutex_lock(req_params->mutex_buffer);
			strcpy(req_params->buffer + (MAX_NAME_LENGTH * (*(req_params->in))), line);
			*(req_params->in) = (*(req_params->in)) + 1 % ARR_SIZE;
			*(req_params->count) = (*(req_params->count)) + 1;
			if(DEBUG_PRINT && DEBUG_PRINT_ADD)printf("Count: %d\n", *(req_params->count));
		pthread_mutex_unlock(req_params->mutex_buffer);
	pthread_mutex_unlock(req_params->mutex_count);
	pthread_cond_signal(req_params->cond_res);
}

// Return non-zero value when no next file or error
int get_next_file(FILE ** input_file, req_params_t * req_params){
	pthread_mutex_lock(req_params->mutex_index);
		if(*(req_params->index) >= *(req_params->argc)){
			if(DEBUG_PRINT && DEBUG_PRINT_NEXT_FILE)printf("get_next_file: Reached end of input files\n");
			pthread_mutex_unlock(req_params->mutex_index);
			return 1;
		}
		(*input_file) = fopen(req_params->argv[*(req_params->index)], "r+");
		if(*input_file == NULL){
			if(DEBUG_PRINT && DEBUG_PRINT_NEXT_FILE)printf("get_next_file: invalid file\n");
			pthread_mutex_unlock(req_params->mutex_index);
			return 1;
		}
		if(DEBUG_PRINT && DEBUG_PRINT_NEXT_FILE)printf("get_next_file: Thread %lu opened %s\n", pthread_self(), req_params->argv[*(req_params->index)]);
		*(req_params->index) = (*(req_params->index)) + 1;
	pthread_mutex_unlock(req_params->mutex_index);
	return 0;
}

int create_req_params(req_params_t * req_params, char ** argv, FILE * log_file, void * buffer, int * index, int * count, int * in, int * argc, pthread_mutex_t * mutex_index, pthread_mutex_t * mutex_count, pthread_mutex_t * mutex_buffer, pthread_mutex_t * mutex_done, pthread_cond_t * cond_req, pthread_cond_t * cond_res, int * done){
	req_params->log_file = log_file;
	req_params->buffer = buffer;
	req_params->argv = argv;
	req_params->index = index;
	req_params->count = count;
	req_params->in = in;
	req_params->argc = argc;
	req_params->mutex_index = mutex_index;
	req_params->mutex_count = mutex_count;
	req_params->mutex_buffer = mutex_buffer;
	req_params->mutex_done = mutex_done;
	req_params->cond_req = cond_req;
	req_params->cond_res = cond_res;
	req_params->done = done;
	return 0;
}

int create_res_params(res_params_t * res_params, FILE * log_file, void * buffer, int * count, int * out, pthread_mutex_t * mutex_count, pthread_mutex_t * mutex_buffer, pthread_mutex_t * mutex_done, pthread_cond_t * cond_req, pthread_cond_t * cond_res, int * done){
	res_params->log_file = log_file;
	res_params->buffer = buffer;
	res_params->count = count;
	res_params->out = out;
	res_params->mutex_count = mutex_count;
	res_params->mutex_buffer = mutex_buffer;
	res_params->mutex_done = mutex_done;
	res_params->cond_res = cond_res;
	res_params->cond_req = cond_req;
	res_params->done = done;
	return 0;
}

int get_req_res_num(long * req_num, long * res_num, char * argv[]){
	
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
	// CHECK FOR ARGUMENT COUNT >= 6

	if(argc < 6){
		printf("Please enter the command as follows:\n");
		printf("multi-lookup <# requester> <# resolver> <requester log> <resolver log> <data file> [ <data file> ...]\n");
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
		if(DEBUG_PRINT && DEBUG_PRINT_THREAD) printf("create_pool: Created thread %d TID %lu\n", i, arr[i]);
	}
	return 0;
}

int join_pool(int num, pthread_t * arr){
	for(int i = 0; i < num; i++){
		if(pthread_join(arr[i], NULL)){
			printf("join_pool: Failed to join Thread %d TID %lu\n", i, arr[i]);
			return -1;
		}
		if(DEBUG_PRINT && DEBUG_PRINT_THREAD) printf("join_pool: Joined thread %d TID %lu\n", i, arr[i]);
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

