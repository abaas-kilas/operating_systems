/***************************/
/* Headers and Definitions */
/***************************/

#include <errno.h>
#include <fcntl.h>
#include <gtk/gtk.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.h"
#include "wrapper.h"

#define MAX_TABS 100                // Tab limit, first tab reserved for controller 
#define MAX_URL 100                 // URL size limit
#define MAX_FAV 100                 // Favorites limit
#define MAX_LABELS 100

comm_channel comm[MAX_TABS];       // Communication pipes, first pair reserved for controller 

char favorites[MAX_FAV][MAX_URL];  // Global favorites array
int num_fav = 0;                   // Number of favorites

typedef struct tab_list {
  int free; // Tab status
  int pid;  // Tab process ID
} tab_list;
tab_list TABS[MAX_TABS]; // Array for keeping tabs on the tabs :)



/**********************/
/* Util tab functions */
/**********************/

// Gets num of open tabs
int get_num_tabs() {
  int i;
  int num = 0;

  for (i = 1; i < MAX_TABS; i++) {
    if (TABS[i].free) {
      continue;
    }
    num++;
  }

  return num;
}

// Gets first free tab
int get_free_tab() {
  int i;

  for (i = 1; i < MAX_TABS; i++) {
    if (TABS[i].free) {
      break;
    }
  }

  return i;
}

// Initializes tab array
void init_tabs() {
  int i;

  for (i = 1; i < MAX_TABS; i++) {
    TABS[i].free = 1;
  }
  TABS[0].free = 0;

  return;
}



/*******************************/
/* Favorite and util functions */
/*******************************/

// Checks if fav exists (max check done in init function)
// Returns -1 if present, 1 if it exceeded the max, or 0 if all good
int fav_ok(char *uri) {
  if (num_fav == MAX_FAV) {
    return 1;
  } else if (on_favorites(uri)) {
    return -1;
  } else {
    return 0;
  }
}

// Initializes favorites array
void init_favorites(char *fname) {
  // Opens favorite file to process
  FILE *file;
  if ((file = fopen(fname, "r")) == NULL) {
    perror("fopen() failed\n");
    exit(1);
  }

  // Populates global array with favorite sites from file and closes
  char uri[MAX_URL];
  while (fgets(uri, MAX_URL, file)) {
    strtok(uri, "\n");
    if (fav_ok(uri) == 1) {  // Max check
      printf("Initialization issue. Favorite limit reached. Line '%d' to end ignored\n", num_fav);
      continue;
    } else if (fav_ok(uri) == -1) {  // Existence check
      printf("Initialization issue. Favorite exists. '%s' ignored\n", uri);
      break;
    } else {
      strcpy(favorites[num_fav++], uri);
    }
  }
  if (fclose(file) != 0) {
    perror("fclose() failed\n");
    exit(1);
  }

  return;
}

// Add new uri to favorites file and the global favorites array
void update_favorites_file(char *uri) {
  // Add uri to favorites file (open, append, close)
  FILE *file;
  if ((file = fopen(".favorites", "a")) == NULL) {
    perror("fopen() failed\n");
    exit(1);
  }
  fprintf(file, "%s\n", uri);
  if (fclose(file) != 0) {
    perror("fclose() failed\n");
    exit(1);
  }

  // Update favorites array with the new favorite
  strtok(uri, "\n");
  strcpy(favorites[num_fav++], uri);

  return;
}

// An fd non-blocking util function
int non_block_pipe(int fd) {
  int flags;
  if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {          // Gets the flags
    perror("fcntl() failed\n");
    return -1;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {   // Set the non-blocking flag
    perror("fcntl() failed\n");
    return -1;
  }
  return 0;
}



/************************************************/
/* Controller and tab command related functions */
/************************************************/

// Checks constraints. Alerts and returns on failures, sends 
// a NEW_URI_ENTERED command through inbound pipe otherwise
void handle_uri(char *uri, int tab_index) {
  if (tab_index < 1 || tab_index >= MAX_TABS || TABS[tab_index].free) {   // out of bounds or not open
    alert((gchar *)"BAD TAB");
    return;
  } else if (strlen(uri) > MAX_URL) {                                     // Too big
    alert((gchar *)"ENTRY TOO LARGE");
    return;
  } else if (bad_format(uri)) {                                           // Missing protocol
    alert((gchar *)"BAD FORMAT");
    return;
  } else if (on_blacklist(uri)) {                                         // Not allowed to open
    alert((gchar *)"BLACKLISTED SITE");
    return;
  } else {                                                                // Else send the command through the pipe
    req_t req;
    req.type = NEW_URI_ENTERED;
    req.tab_index = tab_index;
    strcpy(req.uri, uri);
    if(write(comm[tab_index].inbound[1], &req, sizeof(req_t)) == -1) {
      perror("write() failed\n");
      exit(1);
    }
  }
}

// Callback for URL entry
void uri_entered_cb(GtkWidget *entry, gpointer data) {
  // Check for data
  if (data == NULL) {
    return;
  }

  // Get the tab
  int tab = query_tab_id_for_request(entry, data);

  // Get the URL
  char *uri = get_entered_uri(entry);
  printf("URL entered: '%s'\n", uri);

  // Hands off information to helper to finish
  handle_uri(uri, tab);

  return;
}

// Callback for favorite menu selection
void menu_item_selected_cb(GtkWidget *menu_item, gpointer data) {
  // Check for data
  if (data == NULL) {
    return;
  }

  // Get the tab
  int tab = query_tab_id_for_request(menu_item, data);
  
  // Get the URL (assumes validity) and append "https://" for rendering
  char *basic_uri = (char *)gtk_menu_item_get_label(GTK_MENU_ITEM(menu_item));
  char uri[MAX_URL];
  sprintf(uri, "https://%s", basic_uri);
  printf("URL selected: '%s'\n", uri);

  // Hands off information to helper to finish
  handle_uri(uri, tab);

  return;
}

// Callback when '+ tab' is hit on controller
void new_tab_created_cb(GtkButton *button, gpointer data) {
  // Check for data
  if (data == NULL) {
    return;
  }

  // Check tab limit
  if (get_num_tabs() == MAX_TABS - 1) {
    alert((gchar *)"TAB LIMIT REACHED");
    return;
  }

  // Get first free tab
  int tab = get_free_tab();

  // Create communication pipes for tab
  if (pipe(comm[tab].inbound) == -1) {
    perror("pipe() failed\n");
    exit(1);
  }
  if (pipe(comm[tab].outbound) == -1) {
    perror("pipe() failed\n");
    exit(1);
  }

  // Make the read end fds non-blocking (error checked)
  if (non_block_pipe(comm[tab].inbound[0]) || non_block_pipe(comm[tab].outbound[0])) 
    return;

  // Fork and exec new render tab, controller does tab bookkeeping
  pid_t pid;
  if ((pid = fork()) == -1) {
    perror("fork() failed\n");
    exit(1);
  } else if (pid == 0) {
    char stab[3];                                     // Stringified tab num 
    char pipes[20];                                   // Stringified pipe fds 
    sprintf(stab, "%d", tab);
    sprintf(pipes, "%d %d %d %d", 
            comm[tab].inbound[0],
            comm[tab].inbound[1],
            comm[tab].outbound[0], 
            comm[tab].outbound[1]);
    execl("./render", "render", stab, pipes, NULL);   // Render the tab
    perror("exec() failed\n");
    exit(1);
  } else {
    TABS[tab].free = 0;                               // Controller stores status and process ID of tab
    TABS[tab].pid = pid;
  }

  return;
}

// Function where controller is created and loops continuously to check open communication pipes
int run_control() {
  browser_window *b_window = NULL;
  int i, nRead;
  req_t req;

  // Create controller window
  create_browser(CONTROLLER_TAB, 0, G_CALLBACK(new_tab_created_cb),
                 G_CALLBACK(uri_entered_cb), &b_window, comm[0]);

  // Create favorites menu
  create_browser_menu(&b_window, &favorites, num_fav);

  // Persistent loop
  while (1) {
    process_single_gtk_event();

    // Loop across all pipes from open tabs (including private controller comm[0] pipes)
    for (i = 0; i < MAX_TABS; i++) {
      if (TABS[i].free) 
        continue;
      
      // Check nRead for data
      nRead = read(comm[i].outbound[0], &req, sizeof(req_t));
      if (nRead == -1 && errno == EAGAIN) continue;                // Nothing read, but no failure
      if (nRead == -1 && errno != EAGAIN) {                        // System call 
        perror("read() failed\n");
        exit(1);
      }

      // PLEASE_DIE, send die signal to open tabs then controller dies/exits (loop cuts)
      if (req.type == PLEASE_DIE) {
        for (int j = 1; j < MAX_TABS; j++) {
          if (TABS[j].free) 
            continue;
          req_t tab_req;
          tab_req.type = PLEASE_DIE;
          tab_req.tab_index = j;
          if(write(comm[j].inbound[1], &tab_req, sizeof(req_t)) == -1) {
            perror("write() failed\n");
            exit(1);
          }
        }
        exit(0);
      }

      // TAB_IS_DEAD, kill and wait on its process, close pipes, then set it to free for reuse
      if (req.type == TAB_IS_DEAD) {
        if (kill(TABS[i].pid, SIGKILL) == -1) {
          perror("kill() failed\n");
          exit(1);
        }
        int status;
        if (waitpid(TABS[i].pid, &status, 0) == -1) {
          perror("waitpid() failed\n");
          exit(1);
        }
        if (close(comm[i].inbound[0]) || close(comm[i].inbound[1]) || close(comm[i].outbound[0]) || close(comm[i].outbound[1])) {
          perror("close() failed\n");
          exit(1);
        }
        TABS[i].free = 1;
      }

      // IS_FAV, update favorites file and add it to the menu
      if (req.type == IS_FAV) {
        char *uri = req.uri;
        if (fav_ok(uri) == 1) {
          alert((gchar *)"FAV LIMIT REACHED");
        } else if (fav_ok(uri) == -1) {
          alert((gchar *)"FAV EXISTS");
        } else {
          update_favorites_file(uri);
          add_uri_to_favorite_menu(b_window, uri);
        }
      }
    }
    if(usleep(1000) == -1){   // slow down system to keep things smooth
      perror("usleep() failed\n");
      exit(1);
    }
  }

  return 0;
}

// Main method starts the simulation
int main(int argc, char **argv) {
  // Arguments check
  if (argc != 1) {
    fprintf(stderr, "browser <no_args>\n");
    exit(0);
  }

  // First init tabs, init blacklist, and init favorites
  init_tabs();
  init_blacklist(".blacklist");
  init_favorites(".favorites");

  // Fork to create a controller process
  pid_t pid;
  if ((pid = fork()) == -1) {
    perror("fork() failed\n");
    exit(1);
  } else if (pid == 0) {                          // Create controller pipes & error check non blocking util
    if (pipe(comm[0].inbound) == -1) {
      perror("pipe() failed\n");
      exit(1);
    }
    if (pipe(comm[0].outbound) == -1) {
      perror("pipe() failed\n");
      exit(1);
    }
    if (non_block_pipe(comm[0].inbound[0]) || non_block_pipe(comm[0].outbound[0])) 
      return 1;                                
    run_control();
  } else {                                        // Main process waits on controller
    if (wait(NULL) == -1) {
      perror("wait() failed\n");
      exit(1);
    }
  }
  return 0;
}
