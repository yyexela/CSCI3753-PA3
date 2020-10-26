#include<pthread.h> // For multi-threading
#include<sys/time.h> // For time
#include<unistd.h> // For Access() function
#include"util.h" // For DNS Lookup function, provided by CSCI 3753
#include"multi-lookup.h" // Custom header for this C file

// Main function:
// - Initializes mock global-variables passed
//   as arguments to the thread pools created
// - Checks to see if command line input was correct
//   by checking file names, argument values, and amount
int main(int argc, char * argv[]){
	// Get start time of process
	struct timeval time_start;
	struct timeval time_end;
	if(gettimeofday(&time_start, NULL)){
		printf("Error getting start time\n");
		return -1;
	}

	// FAUX GLOBAL VARIABLES/ Thread arguments
	FILE * req_log = NULL; // Requester log file
	FILE * res_log = NULL; // Resolver log file
	long req_num, res_num; // Number of requesters and resolvers
	// pthread_t * req_pool, res_pool; // Array of TID for requesters and resolvers (DEFINED LATER)
	void * buffer = malloc(MAX_NAME_LENGTH * ARRAY_SIZE * sizeof(char)); // Buffer allocated in heap
	res_params_t res_params; // Struct passed into resolver threads
	req_params_t req_params; // Struct passed into requester threads
	int out = 0; // For producer/requester, buffer index
	int in = 0; // For consumer/resolver, buffer index
	int count = 0; // number of buffer slots filled
	int index = 5; // position of next file input
	int done = 0; // set to 1 when all files are processed

	if(DEBUG_PRINT && DEBUG_PRINT_MALLOC) printf("Malloc: buffer %p size %lu\n", buffer, MAX_NAME_LENGTH * ARRAY_SIZE * sizeof(char));

	// MUTEXES
	pthread_mutex_t mutex_index; // Used by requesters for which files were serviced
	pthread_mutex_t mutex_count; // Used by both thread pools to lock shared variable count
	pthread_mutex_t mutex_buffer; // Used by both thread pools to lock access to shared buffer
	pthread_mutex_t mutex_req_log; // Used by requesters to lock access to log file
	pthread_mutex_t mutex_res_log; // Used by reoslvers to lock access to log file
	if( pthread_mutex_init(&mutex_index, NULL) ||
		pthread_mutex_init(&mutex_count, NULL) ||
		pthread_mutex_init(&mutex_buffer, NULL) ||
		pthread_mutex_init(&mutex_req_log, NULL) ||
		pthread_mutex_init(&mutex_res_log, NULL)){
		printf("Error initializing mutexes\n");
		return -1;
	}

	// CONDITIONAL VARIABLES
	pthread_cond_t cond_req; // Used by resolvers to signal blocked requesters
	pthread_cond_t cond_res; // Used by requesters to signal blockres resolvers
	if( pthread_cond_init(&cond_req, NULL) ||
		pthread_cond_init(&cond_res, NULL)){
		printf("Error initializing conditional variables\n");
		return -1;
	}

	// CHECK COMMAND LINE ARGUMENTS
	if(check_args(argc, argv)) return -1;

	// CHECK FOR NUMBER OF REQUESTERS AND RESOLVERS
	if(get_req_res_num(&req_num, &res_num, argv)) return -1;

	// CHECK FOR LOG FILES
	if(open_log(&req_log, (char *) (argv[3])) != 0 || open_log(&res_log, (char *) (argv[4])) != 0) return -1;

	// POPULATE STRUCTS USED AS ARGS FOR THREAD POOLS
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
	// NOTE: We use mutex_count to ensure we don't have a race
	// condition with the conditional variable
	pthread_mutex_lock(&mutex_count);
		done = 1;
		pthread_cond_broadcast(&cond_res);
	pthread_mutex_unlock(&mutex_count);

	// JOIN CONSUMER THREAD POOL
	if(join_pool(res_num, (pthread_t *) (res_pool))) return -1;

	// FREE MEMORY FROM BUFFER
	free(buffer);

	// FREE MUTEXES AND COND VARS
	if( pthread_mutex_destroy(&mutex_index) ||
		pthread_mutex_destroy(&mutex_count) ||
		pthread_mutex_destroy(&mutex_buffer) ||
		pthread_mutex_destroy(&mutex_req_log) ||
		pthread_mutex_destroy(&mutex_res_log) ||
		pthread_cond_destroy(&cond_req) ||
		pthread_cond_destroy(&cond_res)){
		printf("Error destroying mutexes/conditional variables\n");
		return -1;
	}

	// CLOSE FILES
	if( fclose(res_log) || 
		fclose(req_log)) {
		printf("Error closing log files\n");
		return -1;
	}

	if(DEBUG_PRINT && DEBUG_PRINT_MALLOC) printf("Free: buffer %p\n", buffer);

	if(DEBUG_PRINT && DEBUG_PRINT_DONE) printf("Successfully finished\n");

	// GET END TIME
	if(gettimeofday(&time_end, NULL)){
		printf("Error getting end time\n");
		return -1;
	}

	// PRINT RUN TIME
	printf("./multi-lookup total time is %f seconds\n",
		(double) (((time_end.tv_sec * 1000000 + time_end.tv_usec) -
		(time_start.tv_sec * 1000000 + time_start.tv_usec))/1000000.0));

	return 0;
}

// REQUESTER START FUNCTION
void * req_func(void * ptr){
	// Cast passed in parameters to struct for easy access
	req_params_t * req_params = (req_params_t *) ptr;

	// Create thread-specific variables
	FILE * input_file = NULL; // Current file that is being read
	char * line = (char *) malloc(sizeof(char) * MAX_NAME_LENGTH); // Last line read
	if(DEBUG_PRINT && DEBUG_PRINT_MALLOC) printf("Malloc: req line %p of size %lu\n", line, sizeof(char) * MAX_NAME_LENGTH);
	int file_status = 0; // 
	int serviced = 0; // Number of files serviced

	// Main producer loop
	while(1){
		// Get a file to service
		file_status = get_next_file(&input_file, req_params);
		if(file_status == 1){
			// NO MORE FILES LEFT
			// Print how many files were serviced
			add_to_req_log_final(serviced, req_params);
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
				if(fclose(input_file)) printf("Error closing input file\n");
			}

			// Increment serviced
			serviced++;
		}
	}	
}

// Add line to buffer: requester threads
void add_to_buffer(char * line, req_params_t * req_params){
	// Count variable mutex
	pthread_mutex_lock(req_params->mutex_count);
		// Only add to buffer when there is space, block when there isn't
		while(*(req_params->count) >= ARRAY_SIZE-1){
			pthread_cond_wait(req_params->cond_req, req_params->mutex_count);
		}
		// Add item to buffer
		pthread_mutex_lock(req_params->mutex_buffer);
			if(DEBUG_PRINT && (DEBUG_PRINT_ADD || DEBUG_PRINT_PERF)) printf("Copying str to buffer at index %d offset %d\n", *(req_params->in), MAX_NAME_LENGTH * (*(req_params->in)));
			strncpy(req_params->buffer + (MAX_NAME_LENGTH * (*(req_params->in))), line, MAX_NAME_LENGTH);
			*(req_params->in) = (*(req_params->in) + 1) % ARRAY_SIZE;
			*(req_params->count) = (*(req_params->count)) + 1;
			if(DEBUG_PRINT && (DEBUG_PRINT_ADD || DEBUG_PRINT_PERF))printf("Count: %d\n", *(req_params->count));
		pthread_mutex_unlock(req_params->mutex_buffer);
	pthread_mutex_unlock(req_params->mutex_count);
	// Signal any waiting resolvers
	pthread_cond_signal(req_params->cond_res);
}

// Open the next file for the requesters
// Returns:
// 0  -   valid file, file was opened (increment index)
// 1  -   no more files
// 2  -   invalid file (increment index)
int get_next_file(FILE ** input_file, req_params_t * req_params){
	// Index mutex is used for the requester thread pool to see which files were serviced
	pthread_mutex_lock(req_params->mutex_index);
		if(*(req_params->index) >= *(req_params->argc)){
			// Reached end of command line arguments, no more files to service
			pthread_mutex_unlock(req_params->mutex_index);
			if(DEBUG_PRINT && DEBUG_PRINT_NEXT_FILE)printf("get_next_file: Reached end of input files\n");
			return 1;
		}
		// Open file for reading, start from the beginning of the file
		(*input_file) = fopen(req_params->argv[*(req_params->index)], "r+");
		if(*input_file == NULL){
			// File is invalid/doesn't exist
			*(req_params->index) = (*(req_params->index)) + 1;
			pthread_mutex_unlock(req_params->mutex_index);
			if(DEBUG_PRINT && DEBUG_PRINT_NEXT_FILE)printf("get_next_file: invalid file\n");
			return 2;
		}
		// File was successfully opened
		if(DEBUG_PRINT && DEBUG_PRINT_NEXT_FILE)printf("get_next_file: Thread %lu opened %s\n", pthread_self(), req_params->argv[*(req_params->index)]);
		*(req_params->index) = (*(req_params->index)) + 1;
	pthread_mutex_unlock(req_params->mutex_index);
	return 0;
}

// Used by requesters to read a line of the file they are servicing
// Returns 0 on successful read of line, 1 otherwise
int read_line(char * line, FILE * input_file){
	if(fgets(line, MAX_NAME_LENGTH, input_file) != NULL){
		long bytes = strlen(line);
		// Replace newline with null-terminating character
		if(bytes != 0){
			if( line[bytes-1] == '\n')
				line[bytes-1] = '\0';
		}
		return 0;
	}
	return 1;
}

// Print line to req_log
void add_to_req_log(char * line, req_params_t * req_params){
	pthread_mutex_lock(req_params->mutex_req_log);
		if(DEBUG_PRINT && DEBUG_PRINT_LOGS) printf("req_log: Acquired lock\n");
		fprintf(req_params->log_file, "%s\n", line);
	pthread_mutex_unlock(req_params->mutex_req_log);
}

// Print how many logs the requester thread serviced
void add_to_req_log_final(int serviced, req_params_t * req_params){
	pthread_mutex_lock(req_params->mutex_req_log);
		if(DEBUG_PRINT && DEBUG_PRINT_LOGS) printf("req_log_final: Acquired lock\n");
		fprintf(req_params->log_file, "Thread %lu serviced %d files.\n", pthread_self(), serviced);
	pthread_mutex_unlock(req_params->mutex_req_log);
}

// RESOLVER START FUNCTION
void * res_func(void * ptr){
	// Cast parameters to struct for easy access
	res_params_t * res_params = (res_params_t *) ptr;
	char * line = (char *) malloc(sizeof(char) * MAX_NAME_LENGTH); // Line for getting from buffer and putting to log file
	char ip_addr[MAX_IP_LENGTH]; // Where the result of the DNS lookup function is stored

	if(DEBUG_PRINT && DEBUG_PRINT_MALLOC) printf("Malloc: req line %p of size %lu\n", line, sizeof(char) * MAX_NAME_LENGTH);

	// Main consumer loop
	while(1){
		if(remove_from_buffer((char *) line, res_params)){
			// Free allocated memory and exit when no more producers
			free(line);
			if(DEBUG_PRINT && DEBUG_PRINT_MALLOC) printf("Free: res line %p\n", line);
			return 0;
		}
		if(DEBUG_PRINT && DEBUG_PRINT_REMOVE) printf("Consumer read %s\n", line);

		// Add DNS part and print to log
		if(DISABLE_DNS){
			ip_addr[0] = '\0';
		} else {
			 if(dnslookup(line, ip_addr, MAX_IP_LENGTH) == UTIL_FAILURE){
				printf("Bogus hostname: %s\n", line);
				ip_addr[0] = '\0';
			}
		}

		add_to_res_log((char *) line, (char *) ip_addr, res_params);
		if(DEBUG_PRINT && DEBUG_PRINT_LOGS_STDOUT) printf("Thread %lu reads %s as %s\n", pthread_self(), line, ip_addr);
		
	}
}

// return -1 only when done, return 0 when read item
int remove_from_buffer(char * line, res_params_t * res_params){
	// Lock count mutex, used for count variable
	pthread_mutex_lock(res_params->mutex_count);
		// Remove from buffer when there is something, block otherwise
		while(*(res_params->count) <= 0){
			if(*(res_params->done)){
				pthread_mutex_unlock(res_params->mutex_count);
				return -1;
			}
			pthread_cond_wait(res_params->cond_res, res_params->mutex_count);
		}
		// Remove item from buffer
		pthread_mutex_lock(res_params->mutex_buffer);
			if(DEBUG_PRINT && (DEBUG_PRINT_ADD || DEBUG_PRINT_PERF)) printf("Copying str from buffer at index %d offset %d\n", *(res_params->out), MAX_NAME_LENGTH * (*(res_params->out)));
			strncpy(line, res_params->buffer + (MAX_NAME_LENGTH * (*(res_params->out))), MAX_NAME_LENGTH);
			*(res_params->out) = (*res_params->out + 1) % ARRAY_SIZE;
			*(res_params->count) = (*(res_params->count)) - 1;
			if(DEBUG_PRINT && (DEBUG_PRINT_REMOVE || DEBUG_PRINT_COUNT || DEBUG_PRINT_PERF)) printf("Count: %d\n", *(res_params->count));
		pthread_mutex_unlock(res_params->mutex_buffer);
	pthread_mutex_unlock(res_params->mutex_count);
	// Signal producers there is space in the buffer
	pthread_cond_signal(res_params->cond_req);

	return 0;
}

// Add line to resolver log after DNS lookup
void add_to_res_log(char * name, char * ip_addr, res_params_t * res_params){
	// Lock resolver log mutex
	pthread_mutex_lock(res_params->mutex_res_log);
		if(DEBUG_PRINT && DEBUG_PRINT_LOGS) printf("res_log: Acquired lock\n");
		fprintf(res_params->log_file, "%s,%s\n", name, ip_addr);
	pthread_mutex_unlock(res_params->mutex_res_log);
}

// Get the values of req_num and res_num from command line arguments
int get_req_res_num(long * req_num, long * res_num, char * argv[]){
	*(req_num) = strtol(argv[1], &argv[2]-1,10);
	if(*(req_num) < 0 || *(req_num) > MAX_REQUESTER_THREADS){
		printf("Ensure 1 to %d requesters\n", MAX_REQUESTER_THREADS);
		return -1;
	}
	printf("Number of requester threads is: %ld\n", *(req_num));

	*(res_num) = strtol(argv[2], &argv[3]-1,10);
	if(*(res_num) < 0 || *(res_num) > MAX_RESOLVER_THREADS){
		printf("Ensure 1 to %d resolvers\n", MAX_RESOLVER_THREADS);
		return -1;
	}
	printf("Number of resolver threads is: %ld\n", *(res_num));

	return 0;
}

// Ensure there are the correct number of command line arguments
// and that the log files are different
int check_args(int argc, char * argv[]){
	// CHECK FOR ARGUMENT COUNT >= 6
	if(argc < 6){
		printf("Please enter the command as follows:\n");
		printf("multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ...]\n");
		return -1;
	}

	// CHECK LOG FILES ARE DIFFERENT
	if(strcmp(argv[3], argv[4]) == 0){
		printf("Error: requester log and resolver log files are the same\n");
		return -1;
	}

	return 0;
}

// Create a thread pool with `num` threads, store the TID in `arr`, start the threads
// at function `func` and pass in arguments `params`
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

// Join a created thread pool with `num` threads
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

// Open a log file and save the resulting File *
int open_log(FILE ** log, char * file_name){
	// Check if file exists
	if(access(file_name, F_OK)){
		printf("Log file %s doesn't exist\n", file_name);
		return -1;
	}
	(*log) = fopen(file_name, "w"); // Second option "w" clears the file before writing
	if(log == NULL){
		printf("Error opening %s\n", file_name);
		return -1;
	}

	return 0;
}

