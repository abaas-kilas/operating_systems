/* CSCI-4061 Fall 2022
 * Group Member #1: Abaas Kilas kilas002
 * Group Member #2: Kirill Rakitin rakit006
 * Group Member #3: Jakob Robinson robi1456
 */

// Header Files
#include "wrapper.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <ctype.h>

// Function Definitions
void new_tab_created_cb(GtkButton *button, gpointer data);
int run_control();
int empty_or_whitespace(char *uri);
int on_blacklist(char *uri);
int bad_format(char *uri);
void uri_entered_cb(GtkWidget *entry, gpointer data);
void init_blacklist(char *fname);

// Global Definitions
#define MAX_TAB 100  // Maximum number of tabs allowed
#define MAX_BAD 1000 // Maximum number of URL's in blacklist allowed
#define MAX_URL 100  // Maximum char length of a url allowed

// Global Variables
char blacklist[MAX_BAD][MAX_URL]; // Array of blacklist sites read in from file
pid_t processes[MAX_TAB];         // Array of tab processes from the controller
int tab_count = 0;                // Holds current tab number

/*
 * Name:		          new_tab_created_cb
 *
 * Input arguments:
 *      'button'      - whose click generated this callback
 *			'data'        - auxillary data passed along for handling
 *			                this event.
 *
 * Output arguments:   void
 *
 * Description:        This is the callback function for the 'create_new_tab'
 *			               event which is generated when the user clicks the '+'
 *			               button in the controller-tab. The controller-tab
 *			               redirects the request to the parent (/router) process
 *			               which then creates a new child process for creating
 *			               and managing this new tab. No operation right now.
 */
void new_tab_created_cb(GtkButton *button, gpointer data)
{
}

/*
 * Name:                run_control
 * Output arguments:    void
 * Function:            This function will make a CONTROLLER window and be blocked until the program terminates.
 */
int run_control()
{
  // Init a browser_window object
  browser_window * b_window = NULL;

  // Create controller window with callback function
  create_browser(CONTROLLER_TAB, 0, G_CALLBACK(new_tab_created_cb),
                 G_CALLBACK(uri_entered_cb), &b_window);

  // Enter the GTK infinite loop
  show_browser();
  return 0;
}

/*
    Function: empty_or_whitespace    --> "Check for an empty or whitespace only uri"
    Input:    char* uri              --> "URI to check"
    Returns:  True  (1) if uri is invalid
              False (0) if uri is valid
*/
int empty_or_whitespace(char *uri)
{

  if (strlen(uri) == 0)
  { // Empty case
    return 1;
  }
  int l = strlen(uri), i;
  for (i = 0; i < l; i++)
  {
    if (!(isspace(uri[i])))
    { // Contains a character, terminate the check
      return 0;
    }
  }
  return 1; // Whitespace only case
}

/*
    Function: on_blacklist  --> "Check if the provided URI is in th blacklist"
    Input:    char* uri     --> "URI to check against the blacklist"
    Returns:  True  (1) if uri in blacklist
              False (0) if uri not in blacklist
*/
int on_blacklist(char *uri)
{

  // Skip initial formatting
  char *strip_uri;
  if (uri[7] == '/')
  { // https:// case
    strip_uri = uri + 8;
  }
  else
  { // http:// case
    strip_uri = uri + 7;
  }

  // Add/remove www. part of uri to check against the blacklist as well
  char *www = "www.";
  char *skip;
  char w_uri[MAX_URL];
  if (strlen(uri) > 4 && strncmp(www, strip_uri, strlen(www)) == 0)
  { // The www. case
    skip = strip_uri + 4;
    strcpy(w_uri, skip);
  }
  else
  { // No www. case
    strcpy(w_uri, "www.");
    strcat(w_uri, strip_uri);
  }

  // Search global blacklist array for the two versions of the uri
  for (int i = 0; i < MAX_BAD; i++)
  {
    if (strcmp(blacklist[i], strip_uri) == 0)
    {
      return 1;
    }
    if (strcmp(blacklist[i], w_uri) == 0)
    {
      return 1;
    }
  }
  return 0;
}

/*
    Function: bad_format    --> "Check for a badly formatted url string"
    Input:    char* uri     --> "URI to check if it is bad"
    Returns:  True  (1) if uri is badly formatted
              False (0) if uri is well formatted
*/
int bad_format(char *uri)
{

  // Checks if uri starts with the required format and that there is a valid string directly after
  // (i.e. remaining part is not empty or includes leading space)
  char *http = "http://";
  char *https = "https://";
  if ((strlen(uri) > 7 && !empty_or_whitespace(uri + 7) && strncmp(http, uri, strlen(http)) == 0) ||
      (strlen(uri) > 8 && !empty_or_whitespace(uri + 8) && strncmp(https, uri, strlen(https)) == 0))
  {
    return 0;
  }
  return 1;
}

/*
 * Name:                uri_entered_cb
 *
 * Input arguments:
 *                      'entry'-address bar where the url was entered
 *			                'data'-auxiliary data sent along with the event
 *
 * Output arguments:     void
 *
 * Function:             When the user hits the enter after entering the url
 *			                 in the address bar, 'activate' event is generated
 *			                 for the Widget Entry, for which 'uri_entered_cb'
 *			                 callback is called. Controller-tab captures this event
 *			                 and sends the browsing request to the router(/parent)
 *			                 process.
 */
void uri_entered_cb(GtkWidget *entry, gpointer data)
{

  // Get URL, print it out, alert if empty or too long
  char *uri = get_entered_uri(entry);
  printf("URL entered: '%s'\n", uri);
  if (empty_or_whitespace(uri))
  {
    alert((gchar *)"NOTHING ENTERED");
  }
  else if (strlen(uri) > MAX_URL)
  {
    alert((gchar *)"ENTRY TOO LARGE");
  }
  

  // Format, blacklist, and tab count checking
  else if (bad_format(uri))
  {
    alert((gchar *)"BAD FORMAT");
  }
  else if (on_blacklist(uri))
  {
    alert((gchar *)"BLACKLISTED SITE");
  }
  else if (tab_count >= MAX_TAB)
  {
    alert((gchar *)"TAB LIMIT REACHED");
  }

  // Opening the URL
  else
  {
    pid_t pid;
    if ((pid = fork()) == -1)
    {
      perror("fork() failed\n");
      exit(1);
    }
    else if (pid == 0)
    { // Executes the program responsible for generating the graphical window tab
      char tab[3];
      sprintf(tab, "%d", tab_count);
      execl("./render", "render", tab, uri, NULL);
      perror("exec() failed\n");
      exit(1);
    }
    else
    { // Controller appends child pid to the process array and increments the tab count
      processes[tab_count] = pid;
      tab_count++;
    }
  }
  return;
}

/*
    Function: init_blacklist --> "Open a passed in filepath to a blacklist of url's, read and parse into an array"
    Input:    char* fname    --> "file path to the blacklist file"
    Returns:  void
*/
void init_blacklist(char *fname)
{

  // Opens blacklist file to process
  FILE *file;
  file = fopen(fname, "r");
  if (file == NULL)
  {
    perror("fopen() failed\n");
    exit(1);
  }

  // Populates global array with blacklist sites from file
  char uri[MAX_URL];
  int count = 0;
  while (fgets(uri, MAX_URL, file))
  {
    if(count == MAX_BAD){
      printf("Blacklist limit was reached. Remaining sites will be ignored\n");
      break;
    }
    strcpy(blacklist[count], strtok(uri, "\n"));
    count++;
    
  }

  fclose(file);
  return;
}

/*
    Name: main
    Function: Makes sure a blacklist file is provided and processes it. Creates a process to run the controller
              Controller is waited on and can spawn children processes (tabs). Those processes are killed and
              waited on by the controller before it's own process is killed.
*/
int main(int argc, char **argv)
{

  // Check arguments for blacklist, error and warn if no blacklist
  if (argc != 2)
  {
    fprintf(stderr, "browser <blacklist_file>\n");
    exit(0);
  }

  // Initialize the blacklist of url's
  init_blacklist(argv[1]);

  // Create a process to run controller
  pid_t pid;
  if ((pid = fork()) == -1)
  {
    perror("fork() failed\n");
    exit(1);
  }
  else if (pid == 0)
  {
    run_control();

    // Kill all rendered tabs
    for (int i = 0; i <= tab_count; i++)
    {
      if (kill(processes[i], SIGKILL) == -1)
      {
        perror("kill() failed\n");
        exit(1);
      }
    }

    // Wait on all rendered tabs
    for (int i = 0; i <= tab_count; i++)
    {
      int status;
      if (waitpid(processes[i], &status, 0) == -1)
      {
        perror("waitpid() failed\n");
        exit(1);
      }
    }
  }
  else
  {
    // Wait on controller
    if (wait(NULL) == -1)
    {
      perror("wait() failed\n");
      exit(1);
    }
  }

  return 0;
}
