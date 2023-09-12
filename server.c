#include "server.h"
#define PERM 0644

// Global Variables 
int queue_len           = INVALID;    
int cache_len           = INVALID;  
int num_worker          = INVALID;   
int num_dispatcher      = INVALID;   
FILE *logfile;                    

int cacheIndex      = 0;      
int workerIndex = 0;     
int dispatcherIndex = 0;             
int curequest= 0;                     


pthread_t worker_thread[MAX_THREADS];  
pthread_t dispatcher_thread[MAX_THREADS]; 
int threadID[MAX_THREADS];         


pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;    
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t free_space = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue_full = PTHREAD_COND_INITIALIZER;
request_t req_entries[MAX_QUEUE_LEN];   

cache_entry_t* cache;  // global cache pointer


// Function to check whether the given request is present in cache. Gets cache index or returns INVALID
int getCacheIndex(char *request){
 for(int i = 0; i < cache_len; i++){
   if(cache[i].request != NULL && strcmp(cache[i].request, request) == 0){
    return i;
   }
 }
  return INVALID;
}

// Function to add the request and its file content into the cache
void addIntoCache(char *mybuf, char *memory , int memory_size){
  // init, check if valid request
  char *mal1;char *mal2;
  cache[cacheIndex].len = memory_size;
  if(cache[cacheIndex].request != NULL){
    free(cache[cacheIndex].request);
  }
  if((mal1 = malloc(sizeof(BUFF_SIZE)))==NULL){
    perror("MALLOC FAILED");
  }
  // copy request into cache via mal1, mal2
  strcpy(mal1, mybuf);
  cache[cacheIndex].request = mal1;

  if(cache[cacheIndex].content != NULL){
    free(cache[cacheIndex].content);
  }
  if(( mal2 = malloc(memory_size))==NULL ){
    perror("MALLOC FAILED");
  }
  memcpy(mal2, memory, memory_size);
  cache[cacheIndex].content = mal2;
  // Loop around if cache index exceeded
  if(cacheIndex < cache_len-1){
    cacheIndex++;
  }else{
  cacheIndex = 0;
  }
}

// Clear the memory allocated to the cache
void deleteCache(){
  for(int i = 0; i < cache_len; i++){
    free(cache[i].content);
    free(cache[i].request);
  }
  free(cache);
}

// Allocates and initialize an array of cache entries of length cache size
void initCache(){
  cache = malloc(sizeof(cache_entry_t) * cache_len);
  for(int i = 0; i < cache_len; i++){
    cache[i].len = 0;
    cache[i].request = NULL;
    cache[i].content = NULL;
  }
}

// Get content type of file via info stored in buffer
char* getContentType(char *mybuf) {
  // Initialize content type pointer, find position of the file extension (dot '.')
  char* conType;
  int dotPosition = 0;
  while (mybuf[dotPosition] != '.' && mybuf[dotPosition] != '\0') {
    dotPosition++;
  }

  // Set tester pointer to the file extension (including dot)
  char* tester = mybuf + dotPosition;

  // Determine content type based on the file extension
  if (strcmp(tester, ".html") == 0 || strcmp(tester, ".htm") == 0) {
    conType = "text/html";
  }
  else if (strcmp(tester, ".jpg") == 0) {
    conType = "image/jpeg";
  }
  else if (strcmp(tester, ".gif") == 0) {
    conType = "image/gif";
  }
  else {
    conType = "text/plain"; // Default to plain text if extension is not recognized
  }

  return conType;
}


// Open and read the file from the disk into the memory
int readFromDisk(int fd, char *mybuf, void **memory) {
  int fp;
  if((fp = open(mybuf, O_RDONLY)) == -1){
    fprintf (stderr, "ERROR: Fail to open the file.\n");
    return INVALID;
  }
  // Allocate memory for file & then read file
  struct stat st;
  fstat(fp,&st);
  int fileSize = st.st_size;
  *memory = malloc(fileSize);
  read(fp,*memory, fileSize);

  // Close file and return its size
  if(close(fp) != 0){
    perror("file close failed\n");
  }
  return fileSize;
}

// Recieves path request from client & adds to queue
void * dispatch(void *arg) {

  char fNameArray[BUFF_SIZE];
  request_t theFile;

  while (1) {
    // accept client connection, get request
    theFile.fd = accept_connection();
    if(get_request(theFile.fd, fNameArray) != 0){
      perror("failure to get request");
    }
    char* chopped = fNameArray+1;
    fprintf(stderr, "Dispatcher Received Request: fd[%d] request[%s]\n", theFile.fd, chopped);
    // Adding request into queue:
    theFile.request = malloc(strlen(chopped));
    strcpy(theFile.request, chopped);  // copy request into request struct

    // Lock & wait for available queue space
    if(pthread_mutex_lock(&queue_lock) != 0){
      perror("locking failed\n");
    }
    if(curequest == MAX_QUEUE_LEN){
      if(pthread_cond_wait(&free_space, &queue_lock) != 0){
        perror("wait failed\n");
      }
    }

    // Insert request into queue, update indexing
    req_entries[dispatcherIndex] = theFile;
    curequest ++;
    dispatcherIndex = (dispatcherIndex+1)%queue_len;

    // Release lock
    if(pthread_mutex_unlock(&queue_lock) != 0){
      perror("unlocking failed\n");
    }
    if(pthread_cond_signal(&queue_full) != 0){
      perror("signal failed\n");
    }
 }
}

// Retrieves request from queue, process it, then return result to client
void *worker(void *arg) {
  // Disable GCC warnings for unused variables
  #pragma GCC diagnostic ignored "-Wunused-variable"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic pop 

  // Variable declarations
  int num_request = 0;    // Index for requests (used for logging)
  bool cache_hit = false; 
  int filesize = 0;     
  void *memory = NULL;    // Pointer to hold memory read from disk
  int fd = INVALID;       // File descriptor for incoming request
  char mybuf[BUFF_SIZE];  // Buffer to hold file path from request

  int tid = *(int*)arg;
  char fNameArray[BUFF_SIZE];
  request_t theFile;

  while (1) {
    // Request thread-safe access to the request queue by acquiring the queue_lock
    if(pthread_mutex_lock(&queue_lock) != 0){
      perror("lock failed\n");
    }

    // While the request queue is empty, conditionally wait for the queue to become non-empty
    if(curequest == 0){
      if(pthread_cond_wait(&queue_full, &queue_lock) != 0){
        perror("wait failed\n");
      }
    }

    // Now that unlocked & available, read from the request queue
    strcpy(mybuf, req_entries[workerIndex].request);
    fd = req_entries[workerIndex].fd;

    // Circularly update request queue index
    curequest--;
    workerIndex = (workerIndex + 1) % queue_len;

    // Check for a path with only a "/" and add "index.html" to it if necessary
    if(strcmp(mybuf,"/") == 0){
      strcpy(mybuf,"/index.html");
    }

    // Signal that the request queue has a free slot and release the request queue lock
    if(pthread_cond_signal(&free_space) != 0){
      perror("signal failed\n");
    }
    if(pthread_mutex_unlock(&queue_lock) != 0){
      perror("unlock failed\n");
    }

    // Get data from cache if present, else read from disk
    if(pthread_mutex_lock(&cache_lock) != 0){
      perror("lock failed\n");
    }

    int cacheIndex = getCacheIndex(mybuf);
    
    if(cacheIndex != INVALID){ // Cache hit case
      cache_hit = true;
      filesize = cache[cacheIndex].len;
      memory = cache[cacheIndex].content;
    }
    else{ // Cache miss case
      cache_hit = false;
      int rfd = readFromDisk(fd, mybuf, &memory);
      addIntoCache(mybuf, memory, rfd);
      filesize = rfd;
    }

    // Release locks
    if(pthread_mutex_unlock(&cache_lock) != 0){
      perror("unlock failed\n");
    }
    if(pthread_mutex_lock(&log_lock) != 0){
      perror("lock failed\n");
    }

    // Log the request into a file and terminal
    LogPrettyPrint(logfile, tid, ++num_request, fd, mybuf, filesize, cache_hit);

    if(pthread_mutex_unlock(&log_lock) != 0){
      perror("unlock failed\n");
    }

    // Get content type and return result or error
    char* type = getContentType(mybuf);
    if(filesize < 0){
      return_error(fd, memory);
    }
    else{
      return_result(fd, type, memory, filesize);
    }
  }
}


/**********************************************************************************/

int main(int argc, char **argv) {
  // Error check on the number of arguments
  if (argc != 7) {
    fprintf(stderr, "Usage: %s port path num_dispatcher num_workers queue_length cache_size\n", argv[0]);
    return -1;
  }

  int port = -1;
  char path[PATH_MAX] = "no path set\0";
  // Global vars:
  num_dispatcher = -1; 
  num_worker = -1;  
  queue_len = -1;
  cache_len = -1;  

  // Get and validate input arguments
  port = atoi(argv[1]);
  strcpy(path, argv[2]);
  num_dispatcher = atoi(argv[3]);
  num_worker = atoi(argv[4]);
  queue_len = atoi(argv[5]);
  cache_len = atoi(argv[6]);

  // Perform error checks on input arguments
  if (port < MIN_PORT || port > MAX_PORT) {
    fprintf(stderr, "Port must be between %d and %d\n", MIN_PORT, MAX_PORT);
    exit(-1);
  }

  if (opendir(path) == NULL) {
    fprintf(stderr, "Invalid path: %s\n", path);
    exit(-1);
  }

  if (num_dispatcher < 1 || num_dispatcher > MAX_THREADS) {
    fprintf(stderr, "num_dispatcher must be between 1 and MAX_THREADS\n");
    exit(-1);
  }

  if (num_worker < 1 || num_worker > MAX_THREADS) {
    fprintf(stderr, "num_worker must be between 1 and MAX_THREADS\n");
    exit(-1);
  }

  if (queue_len < 1 || queue_len > MAX_QUEUE_LEN) {
    fprintf(stderr, "queue_len must be between 1 and MAX_QUEUE_LEN\n");
    exit(-1);
  }

  if (cache_len < 1 || cache_len > MAX_CE) {
    fprintf(stderr, "cache_len must be between 1 and MAX_CE\n");
    exit(-1);
  }

  // Print verified arguments
  printf("Arguments Verified:\n\
    Port:           [%d]\n\
    Path:           [%s]\n\
    num_dispatcher: [%d]\n\
    num_workers:    [%d]\n\
    queue_length:   [%d]\n\
    cache_size:     [%d]\n\n", port, path, num_dispatcher, num_worker, queue_len, cache_len);

  // Open log file
  logfile = fopen(LOG_FILE_NAME, "w");
  if (logfile == NULL) {
    perror("Error opening logfile");
    exit(-1);
  }

  // Change the current working directory to the server root directory
  if (chdir(path) == -1) {
    perror("Error changing directory to root directory");
    exit(-1);
  }

  // Initialize cache, port
  initCache();
  init(port);

  // Create dispatcher and worker threads
  int arg_arr[num_worker];
  for (int i = 0; i < num_worker; i++) {
    threadID[i] = i; // Assign worker threads an ID
  }

  for (int i = 0; i < num_worker; i++) {
    if (pthread_create(&(worker_thread[i]), NULL, worker, (void *)&threadID[i]) != 0) {
      fprintf(stderr, "Error creating worker thread %d\n", i);
      continue;
    }
  }

  for (int i = 0; i < num_dispatcher; i++) {
    if (pthread_create(&(dispatcher_thread[i]), NULL, dispatch, (void *)&threadID[i]) != 0) {
      fprintf(stderr, "Error creating dispatcher thread %d\n", i);
      continue;
    }
  }

  // Wait for each of the threads to complete their work
  // Threads (if created) will not exit (see while loop), but this keeps the main from exiting
  int i;
  for (i = 0; i < num_worker; i++) {
    fprintf(stdout, "JOINING WORKER %d\n", i);
    if ((pthread_join(worker_thread[i], NULL)) != 0) {
      fprintf(stderr, "ERROR: Failed to join worker thread %d.\n", i);
    }
  }
  for (i = 0; i < num_dispatcher; i++) {
    fprintf(stdout, "JOINING DISPATCHER %d\n", i);
    if ((pthread_join(dispatcher_thread[i], NULL)) != 0) {
      fprintf(stderr, "ERROR: Failed to join dispatcher thread %d.\n", i);
    }
  }
  deleteCache();
  fprintf(stderr, "SERVER DONE\n");  // Should never be reached
}
