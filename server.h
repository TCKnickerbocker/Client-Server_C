#ifndef _SERVER_H
#define _SERVER_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include "util.h"
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>

// MACROS
#define MIN_PORT 1024  // Since <1024 are protected port numbers
#define MAX_PORT 65535 
#define MAX_THREADS 100 
#define MAX_QUEUE_LEN 100  
#define MAX_CE 100  
#define INVALID -1  // Will be used as our universal failure number
#define INVALID_FLAG 99999  
#define BUFF_SIZE 1024   
#define LOG_FILE_NAME "webserver_log" 

typedef struct request_queue {
  int fd;
  char *request;  // dynamic length URL
} request_t;

typedef struct cache_entry {
  int len;
  char *request; // The URL
  char *content; // The bytes of the file  
} cache_entry_t;


/* -------------------------------------------------------------------------*/
// SERVER FUNCTIONS
/* -------------------------------------------------------------------------*/


/*
 * getCacheIndex - Get the cache index for a given request.
 *
 * Parameters:
 *   request: The request for which to find the cache index.
 *
 * Returns:
 *   The cache index on success, or -1 if the request is not in the cache.
 */
int getCacheIndex(char *request);

/*
 * addIntoCache - Add data into the cache memory.
 *
 * Parameters:
 *   mybuf: Pointer to the data to be added into the cache.
 *   memory: Pointer to the cache memory.
 *   memory_size: The size of the cache memory.
 *
 * This function is used to store data in the cache for faster retrieval.
 */
void addIntoCache(char *mybuf, char *memory, int memory_size);

/*
 * deleteCache - Delete the contents of the cache memory.
 */
void deleteCache();

/*
 * initCache - Initialize the cache memory.
 */
void initCache();

/*
 * getContentType - Get the content type from a given buffer.
 *
 * Parameters:
 *   mybuf: Pointer to the buffer containing the data.
 *
 * Returns:
 *   A pointer to the content type string.
 */
char* getContentType(char *mybuf);

/*
 * readFromDisk - Read data from disk into a buffer.
 *
 * Parameters:
 *   fd: File descriptor for reading from disk.
 *   mybuf: Pointer to the buffer where data will be read.
 *   memory: Pointer to the memory buffer where the data is stored.
 *
 * Returns:
 *   0 on success, nonzero on failure.
 */
int readFromDisk(int fd, char *mybuf, void **memory);

/*
 * dispatch - Dispatcher thread partitions server work.
 *
 * Parameters:
 *   arg: Argument passed to the dispatcher thread.
 *
 * This function handles the distribution of tasks to worker threads.
 */
void* dispatch(void *arg);

/*
 * worker - Worker threads each handle a single request at a time.
 *
 * Parameters:
 *   arg: Argument passed to the worker thread.
 *
 * This function processes a single client request.
 */
void* worker(void *arg);



// PrettyPrint details
void LogPrettyPrint(FILE* to_write, int threadId, int requestNumber, int file_descriptor, char* request_str, int num_bytes_or_error, bool cache_hit){
    if(to_write == NULL){
        printf("[%3d] [%3d] [%3d] [%-30s] [%7d] [%5s]\n", threadId, requestNumber, file_descriptor, request_str, num_bytes_or_error, cache_hit? "TRUE" : "FALSE");
        fflush(stdout);
    }else{
        fprintf(to_write, "[%3d] [%3d] [%3d] [%-30s] [%7d] [%5s]\n", threadId, requestNumber, file_descriptor, request_str, num_bytes_or_error, cache_hit? "TRUE" : "FALSE");
        fflush(to_write);
    }    
}
#endif /* _SERVER_H */