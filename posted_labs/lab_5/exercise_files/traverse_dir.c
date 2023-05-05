#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

/* 
    Function: traverse_dir_core       --> "Recursively print out the information of non-directory files inside a directory"
    Input:    char *root_dir_path     --> "The path of the directory"
    Returns:  void

    TODOs:  1. Open root_dir_path directory
            2. In the while() condition, read the contents of the root_dir_path directory
            3. If child_path is a directory, recursively print out the information of non-directory files inside child_path
            4. Print out the non-directory file's information: including:
                a. file's name
                b. number of hard links of this file
                c. file size
            5. Close root_dir_path directory
*/
void traverse_dir_core(char *root_dir_path) {
    // If root_dir_path is NULL, return
    if (root_dir_path == NULL) {
        return ;
    }
    
    /* TODO 1 */
    DIR* dir; 
    if ((dir = opendir(root_dir_path)) == NULL){
        perror("opendir() failed");
        exit(1);
    };

    // Traverse the root_dir
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) { /* TODO 2 */
        // Skip "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }

        char child_path[strlen(root_dir_path) + strlen(entry->d_name) + 2];
        sprintf(child_path, "%s/%s", root_dir_path, entry->d_name);

        // If child is a directory, go into child directory
        if (entry->d_type == DT_DIR) {
            // Recursively print out the information of non-directory files inside child_path
            /* TODO 3 */
            traverse_dir_core(child_path);
        } else {
            // Print out the file's information
            /* TODO 4 */
            printf("Name: %s\n", entry->d_name);
            struct stat buf;
            if (stat(child_path, &buf) != 0){
                perror("stat() failed");
                exit(1);
            }
            printf("Hard Links: %ld\n", buf.st_nlink);
            printf("Size: %ld\n\n", buf.st_size);
        }
    }

    /* TODO 5 */
    if(closedir(dir) == -1){
        perror("closedir() failed");
        exit(1);
    }

    return;
}

// traverse_dir.c takes the path to a directory as its input and 
// prints out the information of all the non-directory files
// NOTE: please use test_directory to test your traverse_dir.c
// To compile: gcc -o traverse_dir traverse_dir.c
int main(int argc, char** argv) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        exit(1);
    }

    char* root_dir_path = argv[1];

    // Check input
    struct stat root_dir_sb;
    // On success, stat() returns 0
    if (stat(root_dir_path, &root_dir_sb) != 0) {
        fprintf(stderr, "stat() system call failed on %s\n", root_dir_path);
        exit(1);
    }
    // Check whether the input is a directory or not
    if (!(root_dir_sb.st_mode & __S_IFDIR)) {
        fprintf(stderr, "%s is not a directory\n", root_dir_path);
        exit(1);
    }

    // Core logic
    traverse_dir_core(root_dir_path);

    return 0;
}