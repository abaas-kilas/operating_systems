#include "server.h"
#define PERM 0644

//Global Variables [Values Set in main()]
int queue_len           = INVALID;                              //Global integer to indicate the length of the queue
int cache_len           = INVALID;                              //Global integer to indicate the length or # of entries in the cache        
int num_worker          = INVALID;                              //Global integer to indicate the number of worker threads
int num_dispatcher      = INVALID;                              //Global integer to indicate the number of dispatcher threads      
FILE *logfile;                                                  //Global file pointer for writing to log file in worker


/* ************************ Global Hints **********************************/

int cacheIndex = 0;                             //[Cache]           --> When using cache, how will you track which cache entry to evict from array?
int workerIndex = 0;                            //[worker()]        --> How will you track which index in the request queue to remove next?
int dispatcherIndex = 0;                        //[dispatcher()]    --> How will you know where to insert the next request received into the request queue?
int curequest= 0;                               //[multiple funct]  --> How will you update and utilize the current number of requests in the request queue?


pthread_t worker_thread[MAX_THREADS];           //[multiple funct]  --> How will you track the p_thread's that you create for workers?
pthread_t dispatcher_thread[MAX_THREADS];       //[multiple funct]  --> How will you track the p_thread's that you create for dispatchers?
int threadID[MAX_THREADS];                      //[multiple funct]  --> Might be helpful to track the ID's of your threads in a global array


pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;  //What kind of locks will you need to make everything thread safe? [Hint you need multiple]
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t return_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t some_content = PTHREAD_COND_INITIALIZER;  //What kind of CVs will you need  (i.e. queue full, queue empty) [Hint you need multiple]
pthread_cond_t free_space = PTHREAD_COND_INITIALIZER;


request_t req_entries[MAX_QUEUE_LEN];                    //How will you track the requests globally between threads? How will you ensure this is thread safe?
cache_entry_t* cache;                                    //[Cache]  --> How will you read from, add to, etc. the cache? Likely want this to be global

/**********************************************************************************/


/*
  THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGGESTION. FEEL FREE TO MODIFY AS NEEDED
*/


/* ******************************** Cache Code  ***********************************/

// Function to check whether the given request is present in cache
int getCacheIndex(char *request){
  /* TODO (GET CACHE INDEX)
  *    Description:      return the index if the request is present in the cache otherwise return INVALID
  */
  for (int i=0; i < cache_len; i++) {
    if (cache[i].request != NULL && strcmp(cache[i].request, request) == 0) {
      return i;
    }
  }
  return INVALID;
}

// Function to add the request and its file content into the cache
void addIntoCache(char *mybuf, void *memory , int memory_size){
  /* TODO (ADD CACHE)
  *    Description:      It should add the request at an index according to the cache replacement policy
  *                      Make sure to allocate/free memory when adding or replacing cache entries
  */

  cache[cacheIndex].len = memory_size;
  
  if (cache[cacheIndex].request != NULL) {
    free(cache[cacheIndex].request);
    cache[cacheIndex].request = NULL;
  }
  if ((cache[cacheIndex].request = malloc(strlen(mybuf)+1)) == NULL) {
    perror("cache allocation failed\n");
    exit(1);
  }
  strcpy(cache[cacheIndex].request, mybuf);

  if (cache[cacheIndex].content != NULL) {
    free(cache[cacheIndex].content);
    cache[cacheIndex].content = NULL;
  }
  if ((cache[cacheIndex].content = malloc(memory_size)) == NULL) {
    perror("cache allocation failed\n");
    exit(1);
  }
  memcpy(cache[cacheIndex].content, memory, memory_size);

  cacheIndex++;
  cacheIndex %= cache_len;

  return;
}

// Function to clear the memory allocated to the cache
void deleteCache(){
  /* TODO (CACHE)
  *    Description:      De-allocate/free the cache memory
  */
  for (int i=0; i<cache_len; i++) {
    free(cache[i].request);
    free(cache[i].content);
  }

  free(cache);

  return;
}

// Function to initialize the cache
void initCache(){
  /* TODO (CACHE)
  *    Description:      Allocate and initialize an array of cache entries of length cache size
  */
  if ((cache = malloc(cache_len * sizeof(cache_entry_t))) == NULL) {
    perror("cache allocation failed\n");
    exit(1);
  }

  for (int i=0; i<cache_len; i++) {
    cache[i].len = INVALID;
    cache[i].request = NULL;
    cache[i].content = NULL;
  }

  return;
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
char* getContentType(char *mybuf) {
  /* TODO (Get Content Type)
  *    Description:      Should return the content type based on the file type in the request
  *                      (See Section 5 in Project description for more details)
  *    Hint:             Need to check the end of the string passed in to check for .html, .jpg, .gif, etc.
  */
  int i;
  for (i = 0; mybuf[i] != '.'; i++);
  char* ext = mybuf + i;

  char* type;
  if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0 ) {
    type = "text/html";
  }
  else if (strcmp(ext, ".jpg") == 0) {
    type = "image/jpeg";
  }
  else if (strcmp(ext, ".gif") == 0) {
    type = "image/gif";
  }
  else {
    type = "text/plain";
  }
  
  return type;
}

// Function to open and read the file from the disk into the memory. Add necessary arguments as needed
int readFromDisk(char *mybuf, void **memory) {
  /* TODO 
  *    Description:      Find the size of the file you need to read, read all of the contents into a memory location and return the file size
  *    Hint:             Using fstat or fseek could be helpful here
  *                      What do we do with files after we open them?
  */
  FILE *fp;
  if((fp = fopen(mybuf+1, "rb")) == NULL){
    perror("ERROR: Fail to open the file.\n");
    return INVALID;
  }
  
  
  struct stat info;
  if (stat(mybuf+1, &info) != 0) {
      perror("stat failed\n");
      return INVALID;
  }
  int size = info.st_size;

  if ((*memory = malloc(size)) == NULL) {
    perror("content memory allocation failed\n");
    return INVALID;
  }

  if (fread(*memory, size, 1, fp) != 1) {
    perror("reading content failed\n");
    return INVALID;
  }

  if (fclose(fp) != 0) {
    perror("ERROR: Fail to close the file.\n");
    return INVALID;
  }

  return size;
}

/**********************************************************************************/

// Function to receive the path)request from the client and add to the queue
void * dispatch(void *arg) {

  /********************* DO NOT REMOVE SECTION - TOP     *********************/


  /* TODO (B.I)
  *    Description:      Get the id as an input argument from arg, set it to ID
  */
  // int id = *(int *) arg; (unused)


  while (1) {

    /* TODO (FOR INTERMEDIATE SUBMISSION)
    *    Description:      Receive a single request and print the conents of that request
    *                      The TODO's below are for the full submission, you do not have to use a 
    *                      buffer to receive a single request 
    *    Hint:             Helpful Functions: int accept_connection(void) | int get_request(int fd, char *filename
    *                      Recommend using the request_t structure from server.h to store the request. (Refer section 15 on the project write up)
    */

    /* TODO (B.II)
    *    Description:      Accept client connection
    *    Utility Function: int accept_connection(void) //utils.h => Line 24
    */
    int fd;
    if ((fd = accept_connection()) < 0) {
      perror("connection failed\n");
      continue;
    }


    /* TODO (B.III)
    *    Description:      Get request from the client
    *    Utility Function: int get_request(int fd, char *filename); //utils.h => Line 41
    */
    char filename[BUFF_SIZE];
    if (get_request(fd, filename) != 0) {
      perror("get request failed\n");
      continue;
    }
    // fprintf(stderr, "Dispatcher [%d] Received Request: fd[%d] request[%s]\n", id, fd, filename); (unused)


    /* TODO (B.IV)
    *    Description:      Add the request into the queue
    */
        //(1) Copy the filename from get_request into allocated memory to put on request queue
        char *name;
        if ((name = malloc(strlen(filename)+1)) == NULL) {
          perror("filename allocation failed\n");
          exit(1);
        }
        strcpy(name, filename);

        //(2) Request thread safe access to the request queue
        if (pthread_mutex_lock(&queue_lock) != 0) {
          perror("locking failed\n");
          exit(1);
        }

        //(3) Check for a full queue... wait for an empty one which is signaled from req_queue_notfull
        while (curequest == queue_len) {
          if (pthread_cond_wait(&free_space, &queue_lock) != 0) {
            perror("wait failed\n");
            exit(1);
          }
        }

        //(4) Insert the request into the queue
        req_entries[dispatcherIndex].request = name;
        req_entries[dispatcherIndex].fd = fd;

        //(5) Update the queue index in a circular fashion
        curequest++;
        dispatcherIndex++;
        dispatcherIndex %= queue_len;

        //(6) Release the lock on the request queue and signal that the queue is not empty anymore
        if (pthread_cond_signal(&some_content) != 0) {
          perror("signal failed\n");
          exit(1);
        }
        if (pthread_mutex_unlock(&queue_lock) != 0) {
          perror("unlocking failed\n");
          exit(1);
        }
  }


  // Will never be reached at the moment
  pthread_exit(NULL);
  return NULL;
}

/**********************************************************************************/
// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg) {
  /********************* DO NOT REMOVE SECTION - BOTTOM      *********************/


  // Helpful/Suggested Declarations
  int num_request = 0;                                    //Integer for tracking each request for printing into the log
  bool cache_hit  = false;                                //Boolean flag for tracking cache hits or misses if doing 
  int filesize    = 0;                                    //Integer for holding the file size returned from readFromDisk or the cache
  void *memory    = NULL;                                 //memory pointer where contents being requested are read and stored
  int fd          = INVALID;                              //Integer to hold the file descriptor of incoming request
  char mybuf[BUFF_SIZE];                                  //String to hold the file path from the request


  /* TODO (C.I)
  *    Description:      Get the id as an input argument from arg, set it to ID
  */
  int id = *(int *) arg;


  while (1) { 
    
    /* TODO (C.II)
    *    Description:      Get the request from the queue and do as follows
    */
        //(1) Request thread safe access to the request queue by getting the req_queue_mutex lock
        if (pthread_mutex_lock(&queue_lock) != 0) {
          perror("locking failed\n");
          exit(1);
        }

        //(2) While the request queue is empty conditionally wait for the request queue lock once the not empty signal is raised
        while (curequest == 0) {
          if (pthread_cond_wait(&some_content, &queue_lock) != 0) {
            perror("wait failed\n");
            exit(1);
          }
        }

        //(3) Now that you have the lock AND the queue is not empty, read from the request queue
        strcpy(mybuf, req_entries[workerIndex].request);
        fd = req_entries[workerIndex].fd;

        //(4) Update the request queue remove index in a circular fashion
        curequest--;
        workerIndex++;
        workerIndex %= queue_len;

        //(5) Check for a path with only a "/" if that is the case add index.html to it
        if (strcmp(mybuf, "/") == 0) {
          strcpy(mybuf, "/index.html");
        }

        //(6) Fire the request queue not full signal to indicate the queue has a slot opened up and release the request queue lock
        if (pthread_cond_signal(&free_space) != 0) {
          perror("signal failed\n");
          exit(1);
        }
        if (pthread_mutex_unlock(&queue_lock) != 0) {
          perror("unlocking failed\n");
          exit(1);
        }


    /* TODO (C.III)
    *    Description:      Get the data from the disk or the cache 
    *    Local Function:   int readFromDisk(int fd, char *mybuf, void **memory);
    *                      int getCacheIndex(char *request);  
    *                      void addIntoCache(char *mybuf, char *memory , int memory_size);  
    */
    if (pthread_mutex_lock(&cache_lock) != 0) {
      perror("cache locking failed\n");
      exit(1);
    }
    int idx;
    if ((idx = getCacheIndex(mybuf)) == INVALID) {
      cache_hit = false;
      filesize = readFromDisk(mybuf, &memory);
      if (filesize != INVALID) {
        addIntoCache(mybuf, memory, filesize);
      }
    }
    else {
      cache_hit = true;
      memory = cache[idx].content;
      filesize = cache[idx].len;
    }
    if (pthread_mutex_unlock(&cache_lock) != 0) {
      perror("cache unlocking failed\n");
      exit(1);
    }


    /* TODO (C.IV)
    *    Description:      Log the request into the file and terminal
    *    Utility Function: LogPrettyPrint(FILE* to_write, int threadId, int requestNumber, int file_descriptor, char* request_str, int num_bytes_or_error, bool cache_hit);
    *    Hint:             Call LogPrettyPrint with to_write = NULL which will print to the terminal
    *                      You will need to lock and unlock the logfile to write to it in a thread safe manor
    */
    if (pthread_mutex_lock(&log_lock) != 0) {
      perror("log locking failed\n");
      exit(1);
    }
    num_request++;
    LogPrettyPrint(logfile, id, num_request, fd, mybuf, filesize, cache_hit);
    LogPrettyPrint(NULL, id, num_request, fd, mybuf, filesize, cache_hit);
    if (pthread_mutex_unlock(&log_lock) != 0) {
      perror("log unlocking failed\n");
      exit(1);
    }


    /* TODO (C.V)
    *    Description:      Get the content type and return the result or error
    *    Utility Function: (1) int return_result(int fd, char *content_type, char *buf, int numbytes); //look in utils.h 
    *                      (2) int return_error(int fd, char *buf); //look in utils.h 
    */
    if (pthread_mutex_lock(&return_lock) != 0) {
      perror("return locking failed\n");
      exit(1);
    }
    if (filesize == INVALID) {
      char* err = "ERROR: failed to process request. Could be a file i/o error. See Log";
      if (return_error(fd, err) != 0) {
        perror("return error failed\n");
        break;
      }
      free(memory);
    }
    else {
      char* type = getContentType(mybuf);
      if (return_result(fd, type, memory, filesize) != 0) {
        perror("return result failed\n");
        break;
      }
    }
    if (pthread_mutex_unlock(&return_lock) != 0) {
      perror("return unlocking failed\n");
      exit(1);
    }
  }


  // Will never be reached at the moment, unless a thread fails to return result or error. In that case the thread is exited
  pthread_exit(NULL);
  return NULL;
}

/**********************************************************************************/

int main(int argc, char **argv) {

  /********************* Dreturn resulfO NOT REMOVE SECTION - TOP     *********************/
  // Error check on number of arguments
  if(argc != 7){
    fprintf(stderr, "usage: %s port path num_dispatcher num_workers queue_length cache_size\n", argv[0]);
    exit(1);
  }


  /********************* DO NOT REMOVE SECTION - BOTTOM  *********************/
  /* TODO (A.I)
  *    Description:      Get the input args --> (1) port (2) path (3) num_dispatcher (4) num_workers  (5) queue_length (6) cache_size
  */
  int port            = atoi(argv[1]);
  char path[PATH_MAX];  strcpy(path, argv[2]);
  num_dispatcher      = atoi(argv[3]);                               //global variable
  num_worker          = atoi(argv[4]);                               //global variable
  queue_len           = atoi(argv[5]);                               //global variable
  cache_len           = atoi(argv[6]);                               //global variable


  /* TODO (A.II)
  *    Description:     Perform error checks on the input arguments
  *    Hints:           (1) port: {Should be >= MIN_PORT and <= MAX_PORT} | (2) path: {Consider checking if path exists (or will be caught later)}
  *                     (3) num_dispatcher: {Should be >= 1 and <= MAX_THREADS} | (4) num_workers: {Should be >= 1 and <= MAX_THREADS}
  *                     (5) queue_length: {Should be >= 1 and <= MAX_QUEUE_LEN} | (6) cache_size: {Should be >= 1 and <= MAX_CE}
  */
  if (port < MIN_PORT || port > MAX_PORT) {
    fprintf(stderr, "port out of bounds [%d - %d]\n", MIN_PORT, MAX_PORT);
    exit(1);
  }
  struct stat info;
  if (stat(path, &info) != 0 && errno == ENOENT){
    fprintf(stderr, "path '%s' does not exist ]\n", path);
    exit(1);
  }
  if (num_dispatcher < 1 || num_dispatcher > MAX_THREADS) {
    fprintf(stderr, "num_dispatcher out of bounds [1 - %d]\n", MAX_THREADS);
    exit(1);
  }
  if (num_worker < 1 || num_worker > MAX_THREADS) {
    fprintf(stderr, "num_worker out of bounds [1 - %d]\n", MAX_THREADS);
    exit(1);
  }
  if (queue_len < 1 || queue_len > MAX_QUEUE_LEN) {
    fprintf(stderr, "queue_len out of bounds [1 - %d]\n", MAX_QUEUE_LEN);
    exit(1);
  }
  if (cache_len < 1 || cache_len > MAX_CE) {
    fprintf(stderr, "cache_len out of bounds [1 - %d]\n", MAX_CE);
    exit(1);
  }


  /********************* DO NOT REMOVE SECTION - TOP    *********************/
  printf("Arguments Verified:\n\
    Port:           [%d]\n\
    Path:           [%s]\n\
    num_dispatcher: [%d]\n\
    num_workers:    [%d]\n\
    queue_length:   [%d]\n\
    cache_size:     [%d]\n\n", port, path, num_dispatcher, num_worker, queue_len, cache_len);
  /********************* DO NOT REMOVE SECTION - BOTTOM  *********************/


  /* TODO (A.III)
  *    Description:      Open log file
  *    Hint:             Use Global "File* logfile", use definition "LOG_FILE_NAME" as the name, what open flags do you want?
  */
  if ((logfile = fopen(LOG_FILE_NAME, "w+")) == NULL) {
    perror("file opening failed\n");
    exit(1);
  }


  /* TODO (A.IV)
  *    Description:      Change the current working directory to server root directory
  *    Hint:             Check for error!
  */
  if (chdir(path) != 0) {
    perror("change directory failed\n");
    exit(1);
  }


  /* TODO (A.V)
  *    Description:      Initialize cache  
  *    Local Function:   void    initCache();
  */
  initCache();


  /* TODO (A.VI)
  *    Description:      Start the server
  *    Utility Function: void init(int port); //look in utils.h 
  */
  init(port);


  /* TODO (A.VII)
  *    Description:      Create dispatcher and worker threads 
  *    Hints:            Use pthread_create, you will want to store pthread's globally
  *                      You will want to initialize some kind of global array to pass in thread ID's
  *                      How should you track this p_thread so you can terminate it later? [global]
  */
  int i;
  for(i = 0; i < num_worker; i++) {
    threadID[i] = i;
    if(pthread_create(&(worker_thread[i]), NULL, worker, (void *) &threadID[i]) != 0) {
      fprintf(stderr, "ERROR : Fail to create worker thread    \t[%2d]\n", i);
      exit(1);
    }
    else {
      printf("CREATED WORKER    \t[%2d]\n", i);
    }
  }
  for(i = 0; i < num_dispatcher; i++) {
    threadID[i] = i;
    if(pthread_create(&(dispatcher_thread[i]), NULL, dispatch, (void *) &threadID[i]) != 0) {
      fprintf(stderr, "ERROR : Fail to create dispatcher thread\t[%2d]\n", i);
      exit(1);
    }
    else {
      printf("CREATED DISPATCHER\t[%2d]\n", i);
    }
  }
  

  // Wait for each of the threads to complete their work
  // Threads (if created) will not exit (see while loop), but this keeps main from exiting
  for (i = 0; i < num_dispatcher; i++) {
    if((pthread_join(dispatcher_thread[i], NULL)) != 0){
      fprintf(stderr, "ERROR : Fail to join dispatcher thread %d\n", i);
      exit(1);
    }
    else {
      printf("JOINED DISPATCHER \t[%2d]\n", i);
    }
  }
  for (i = 0; i < num_worker; i++) {
    if((pthread_join(worker_thread[i], NULL)) != 0){
      fprintf(stderr, "ERROR : Fail to join worker thread %d\n", i);
      exit(1);
    }
    else {
      printf("JOINED WORKER     \t[%2d]\n", i);
    }
  }

  
  // Will never be reached at the moment
  fprintf(stderr, "SERVER DONE \n");  
  if (fclose(logfile) != 0) {
    perror("file closing failed\n");
    exit(1);
  }
  deleteCache();
  

  return EXIT_SUCCESS;
}
