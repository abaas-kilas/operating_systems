## Project #4  CSCI-4061 Fall 2022

 ### Group Member #1: Jakob Robinson robi1456
 * Contributions: github setup, prelim (init/accept), error checking/style

 ### Group Member #2: Kirill Rakitin rakit006
 * Contributions: prelim (init/accept), returning (result/error), error checking/style

 ### Group Member #3: Abaas Kilas kilas002
 * Contributions: prelim (init/accept), get request, error checking/style

 ### Notes:
 * Wget does not send the raw url. If it contains a ".." in the path it will process that before sending the url to
   get_request. Same with doing so on a web browser. Our test isn't able to catch that error, but it is there. 
 * The "//" in a url results in Wget retrying again and again on the url. Unsure if this is the expected behavior
 * Omitted the flag and ret_val variables in the init function. Did not see use for it, and from our testing the 
   program seems to work as expected