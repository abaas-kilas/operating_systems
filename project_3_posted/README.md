## Project #3  CSCI-4061 Fall 2022
 
 # Group Member #1: Abaas Kilas kilas002
 * Contributions: preliminary/main, dispatch/worker synchronization, error checking and style
 
 # Group Member #2: Kirill Rakitin rakit006
 * Contributions: preliminary/main, allocations and content tasks, error checking and style
 
 # Group Member #3: Jakob Robinson robi1456
 * Contributions: preliminary/main, cache functionality, error checking and style

 # Notes:
 * We used FIFO policy for cache like the request queue. The earliest cache entry, say 0, will 
   be the first evicted when the cache fills up and needs to replace an index. It will be 1 on 
   the next run, 2 the next, and so on
 
 * We added 2 locks to the provided 2. The 4 are the queue lock, cache lock, log lock, and 
   return lock (for when we return results or error to the client)
 
 * We added some specifiers for the log fields so that they were more easily readable. Also 
   modified the memory arg in addIntoCache to be a void pointer and omitted the fd arg in 
   readFromDisk as it was not used (we chose stat over fstat since it was giving us trouble).

 * Did not use PERM definition in server code, and also made slight changes to the Makefile so 
   that it was consistent 