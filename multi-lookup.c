#include<pthread.h>
#include <unistd.h> // TODO: Remove when done, for sleep(n)
#include<sys/time.h> // For time
#include"util.h"
#include"multi-lookup.h"

int main(int argc, char * argv[]){
	struct timeval time_start;
	struct timeval time_end;
	if(gettimeofday(&time_start, NULL)){
		printf("Error getting start time\n");
		return -1;
	}

	// VARIABLES

	FILE * req_log = NULL;
	FILE * res_log = NULL;
	long req_num, res_num;
	// pthread_t * req_pool, res_pool; // DEFINED LATER
	void * buffer = malloc(MAX_NAME_LENGTH * ARR_SIZE * sizeof(char));

	if(DEBUG_PRINT && DEBUG_PRINT_MALLOC) printf("Malloc: buffer %p size %lu\n", buffer, MAX_NAME_LENGTH * ARR_SIZE * sizeof(char));

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
	pthread_mutex_t mutex_req_log;
	pthread_mutex_t mutex_res_log;
	pthread_mutex_init(&mutex_index, NULL);
	pthread_mutex_init(&mutex_count, NULL);
	pthread_mutex_init(&mutex_buffer, NULL);
	pthread_mutex_init(&mutex_done, NULL);
	pthread_mutex_init(&mutex_req_log, NULL);
	pthread_mutex_init(&mutex_res_log, NULL);

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

	// CREATE ARGS FOR THREAD POOLS VIA STRUCTS

	req_params.log_file = req_log;
	req_params.buffer = buffer;
	req_params.argv = argv;
	req_params.index = &index;
	req_params.count = &count;
	req_params.in = &in;
	req_params.argc = &argc;
	req_params.mutex_index = &mutex_index;
	req_params.mutex_count = &mutex_count;
	req_params.mutex_buffer = &mutex_buffer;
	req_params.mutex_done = &mutex_done;
	req_params.mutex_req_log = &mutex_req_log;
	req_params.cond_req = &cond_req;
	req_params.cond_res = &cond_res;
	req_params.done = &done;

	res_params.log_file = res_log;
	res_params.buffer = buffer;
	res_params.count = &count;
	res_params.out = &out;
	res_params.mutex_count = &mutex_count;
	res_params.mutex_buffer = &mutex_buffer;
	res_params.mutex_done = &mutex_done;
	res_params.mutex_res_log = &mutex_res_log;
	res_params.cond_res = &cond_res;
	res_params.cond_req = &cond_req;
	res_params.done = &done;

	// CREATE POOLS
	pthread_t req_pool[req_num];
	pthread_t res_pool[res_num];
	if( create_pool(req_num, (pthread_t *) (req_pool), req_func, (void *) (&req_params)) || 
		create_pool(res_num, (pthread_t * ) (res_pool), res_func, (void *) (&res_params))) return -1;

	// JOIN PRODUCER THREAD POOL
	if(join_pool(req_num, (pthread_t *) (req_pool))) return -1;

	// INFORM CONSUMERS TO EXIT
	pthread_mutex_lock(&mutex_buffer);
			pthread_mutex_lock(&mutex_done);
				done = 1;
				pthread_cond_broadcast(&cond_res);
			pthread_mutex_unlock(&mutex_done);
	pthread_mutex_unlock(&mutex_buffer);

	// JOIN CONSUMER THREAD POOL
	if(join_pool(res_num, (pthread_t *) (res_pool))) return -1;

	// FREE MEMORY FROM BUFFER
	free(buffer);

	// FREE MUTEXES AND COND VARS
	pthread_mutex_destroy(&mutex_index);
	pthread_mutex_destroy(&mutex_count);
	pthread_mutex_destroy(&mutex_buffer);
	pthread_mutex_destroy(&mutex_done);
	pthread_mutex_destroy(&mutex_req_log);
	pthread_mutex_destroy(&mutex_res_log);
	pthread_cond_destroy(&cond_req);
	pthread_cond_destroy(&cond_res);

	// CLOSE FILES
	fclose(res_log);
	fclose(req_log);

	if(DEBUG_PRINT && DEBUG_PRINT_MALLOC) printf("Free: buffer %p\n", buffer);

	if(DEBUG_PRINT && DEBUG_PRINT_DONE) printf("Successfully finished\n");

	// PRINT RUN TIME

	if(gettimeofday(&time_end, NULL)){
		printf("Error getting end time\n");
		return -1;
	}
	
	printf("./multi-lookup ran for %f seconds\n", (double) (((time_end.tv_sec * 1000000 + time_end.tv_usec) - (time_start.tv_sec * 1000000 + time_start.tv_usec))/1000000.0));

	return 0;
}

void * req_func(void * ptr){
	// Cast parameters to struct
	req_params_t * req_params = (req_params_t *) ptr;

	// Create thread-specific variables
	FILE * input_file = NULL;
	char * line = (char *) malloc(sizeof(char) * MAX_NAME_LENGTH);
	if(DEBUG_PRINT && DEBUG_PRINT_MALLOC) printf("Malloc: req line %p of size %lu\n", line, sizeof(char) * MAX_NAME_LENGTH);
	int file_status = 0;
	int serviced = 0;

	// Main producer loop
	while(1){
		// Get a file
		file_status = get_next_file(&input_file, req_params);
		if(file_status == 1){
			// NO MORE FILES LEFT

			// Print how many files were serviced
			strncpy(line, "T", MAX_NAME_LENGTH);
			add_to_req_log_final(serviced, req_params);
			// Free and exit when no more files
			free(line);
			if(DEBUG_PRINT && DEBUG_PRINT_MALLOC) printf("Free: req line %p\n", line);
			return 0;
		} else if(file_status == 2){
			// FILE DOESN'T EXIST

			// Increment serviced
			serviced++;
		} else if (file_status == 0) {
			// FILE EXISTS AND OPENED

			// Produce lines from the input file
			while(read_line((char *) line, input_file) == 0){
				if(DEBUG_PRINT && DEBUG_PRINT_LOGS_STDOUT) printf("Thread %lu reads %s\n", pthread_self(), line);
				// Put lines into the buffer
				add_to_buffer((char *) line, req_params);
				// Put names into req_log
				add_to_req_log((char *) line, req_params);
			}

			// Close opened file
			if(input_file != NULL){
				if(DEBUG_PRINT && DEBUG_PRINT_NEXT_FILE)printf("Thread %lu closed a file\n", pthread_self());
				fclose(input_file);
			}

			// Increment serviced
			serviced++;
		}
	}	
	printf("SHOULD NEVER BE HERE\n");
	free(line);
	if(DEBUG_PRINT && DEBUG_PRINT_MALLOC) printf("Free: req line %p\n", line);
	return 0;
}

void * res_func(void * ptr){
	// Cast parameters to struct
	res_params_t * res_params = (res_params_t *) ptr;
	char * line = (char *) malloc(sizeof(char) * MAX_NAME_LENGTH);
	char ip_addr[MAX_IP_LENGTH];
	if(DEBUG_PRINT && DEBUG_PRINT_MALLOC) printf("Malloc: req line %p of size %lu\n", line, sizeof(char) * MAX_NAME_LENGTH);

	// Main consumer loop
	while(1){
		if(remove_from_buffer((char *) line, res_params)){
			// Free and exit when no more producers
			free(line);
			if(DEBUG_PRINT && DEBUG_PRINT_MALLOC) printf("Free: res line %p\n", line);
			return 0;
		}
		if(DEBUG_PRINT && DEBUG_PRINT_REMOVE) printf("Consumer read %s\n", line);

		// Add DNS part and print to log

		if(dnslookup(line, ip_addr, MAX_IP_LENGTH)){
			if(DEBUG_PRINT && DEBUG_PRINT_DNS) printf("DNS for %s failed\n", line);
			ip_addr[0] = '\0';
		}

		add_to_res_log((char *) line, (char *) ip_addr, res_params);
		if(DEBUG_PRINT && DEBUG_PRINT_LOGS_STDOUT) printf("Thread %lu reads %s as %s\n", pthread_self(), line, ip_addr);
		
	}


	printf("SHOULD NEVER BE HERE\n");
	free(line);
	if(DEBUG_PRINT && DEBUG_PRINT_MALLOC) printf("Free: res line %p\n", line);
	return 0;
}

void add_to_res_log(char * name, char * ip_addr, res_params_t * res_params){
	pthread_mutex_lock(res_params->mutex_res_log);
		if(DEBUG_PRINT && DEBUG_PRINT_LOGS) printf("res_log: Acquired lock\n");
		fprintf(res_params->log_file, "%s,%s\n", name, ip_addr);
	pthread_mutex_unlock(res_params->mutex_res_log);
}

void add_to_req_log(char * line, req_params_t * req_params){
	pthread_mutex_lock(req_params->mutex_req_log);
		if(DEBUG_PRINT && DEBUG_PRINT_LOGS) printf("req_log: Acquired lock\n");
		fprintf(req_params->log_file, "%s\n", line);
	pthread_mutex_unlock(req_params->mutex_req_log);
}

void add_to_req_log_final(int serviced, req_params_t * req_params){
	pthread_mutex_lock(req_params->mutex_req_log);
		if(DEBUG_PRINT && DEBUG_PRINT_LOGS) printf("req_log_final: Acquired lock\n");
		fprintf(req_params->log_file, "Thread %lu serviced %d files.\n", pthread_self(), serviced);
	pthread_mutex_unlock(req_params->mutex_req_log);
}

// return 1 only when done, return 0 when read item
int remove_from_buffer(char * line, res_params_t * res_params){
	pthread_mutex_lock(res_params->mutex_count);
		while(*(res_params->count) <= 0){
			pthread_mutex_lock(res_params->mutex_done);
				if(*(res_params->done)){
					pthread_mutex_unlock(res_params->mutex_done);
					pthread_mutex_unlock(res_params->mutex_count);
					return 1;
				}
			pthread_mutex_unlock(res_params->mutex_done);
			pthread_cond_wait(res_params->cond_res, res_params->mutex_count);
		}
		pthread_mutex_lock(res_params->mutex_buffer);
			if(DEBUG_PRINT && DEBUG_PRINT_ADD) printf("Copying str from buffer at index %d offset %d\n", *(res_params->out), MAX_NAME_LENGTH * (*(res_params->out)));
			strncpy(line, res_params->buffer + (MAX_NAME_LENGTH * (*(res_params->out))), MAX_NAME_LENGTH);
			*(res_params->out) = (*res_params->out + 1) % ARR_SIZE;
			*(res_params->count) = (*(res_params->count)) - 1;
			if(DEBUG_PRINT && DEBUG_PRINT_REMOVE && DEBUG_PRINT_COUNT) printf("Count: %d\n", *(res_params->count));
		pthread_mutex_unlock(res_params->mutex_buffer);
	pthread_mutex_unlock(res_params->mutex_count);
	pthread_cond_signal(res_params->cond_req);

	return 0;
}

// Returns 0 on successful line, 1 otherwise
int read_line(char * line, FILE * input_file){
	if(fgets(line, MAX_NAME_LENGTH, input_file) != NULL){
		long bytes = strlen(line);
		if(bytes != 0){
			line[bytes-1] = '\0';
		}
		return 0;
	}
	return 1;
}

void add_to_buffer(char * line, req_params_t * req_params){
	pthread_mutex_lock(req_params->mutex_count);
		while(*(req_params->count) >= ARR_SIZE-1){
			pthread_cond_wait(req_params->cond_req, req_params->mutex_count);
		}
		pthread_mutex_lock(req_params->mutex_buffer);
			if(DEBUG_PRINT && DEBUG_PRINT_ADD) printf("Copying str to buffer at index %d offset %d\n", *(req_params->in), MAX_NAME_LENGTH * (*(req_params->in)));
			strncpy(req_params->buffer + (MAX_NAME_LENGTH * (*(req_params->in))), line, MAX_NAME_LENGTH);
			*(req_params->in) = (*(req_params->in) + 1) % ARR_SIZE;
			*(req_params->count) = (*(req_params->count)) + 1;
			if(DEBUG_PRINT && DEBUG_PRINT_ADD)printf("Count: %d\n", *(req_params->count));
		pthread_mutex_unlock(req_params->mutex_buffer);
	pthread_mutex_unlock(req_params->mutex_count);
	pthread_cond_signal(req_params->cond_res);
}

// Returns:
// 0  -   valid file, opened
// 1  -   when no next file
// 2  -   invalid file
int get_next_file(FILE ** input_file, req_params_t * req_params){
	pthread_mutex_lock(req_params->mutex_index);
		if(*(req_params->index) >= *(req_params->argc)){
			pthread_mutex_unlock(req_params->mutex_index);
			if(DEBUG_PRINT && DEBUG_PRINT_NEXT_FILE)printf("get_next_file: Reached end of input files\n");
			return 1;
		}
		(*input_file) = fopen(req_params->argv[*(req_params->index)], "r+");
		if(*input_file == NULL){
			*(req_params->index) = (*(req_params->index)) + 1;
			pthread_mutex_unlock(req_params->mutex_index);
			if(DEBUG_PRINT && DEBUG_PRINT_NEXT_FILE)printf("get_next_file: invalid file\n");
			return 2;
		}
		if(DEBUG_PRINT && DEBUG_PRINT_NEXT_FILE)printf("get_next_file: Thread %lu opened %s\n", pthread_self(), req_params->argv[*(req_params->index)]);
		*(req_params->index) = (*(req_params->index)) + 1;
	pthread_mutex_unlock(req_params->mutex_index);
	return 0;
}

int get_req_res_num(long * req_num, long * res_num, char * argv[]){
	
	*(req_num) = strtol(argv[1], &argv[2]-1,10);
	if(*(req_num) < 0 || *(req_num) > MAX_REQUESTER_THREADS){
		printf("Ensure 1 to %d requesters\n", MAX_REQUESTER_THREADS);
		return -1;
	}
	if(DEBUG_PRINT && DEBUG_PRINT_POOLS) printf("Requesters: %ld\n", *(req_num));

	*(res_num) = strtol(argv[2], &argv[3]-1,10);
	if(*(res_num) < 0 || *(res_num) > MAX_RESOLVER_THREADS){
		printf("Ensure 1 to %d resolvers\n", MAX_RESOLVER_THREADS);
		return -1;
	}
	if(DEBUG_PRINT && DEBUG_PRINT_POOLS) printf("Resolvers: %ld\n", *(res_num));

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
	(*log) = fopen(file_name, "w");
	if(log == NULL){
		printf("Error opening %s\n", file_name);
		return -1;
	}

	return 0;
}

