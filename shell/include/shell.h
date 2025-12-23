#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <strings.h> // For strcasecmp

// --- Constants ---
#define MAX_CMD_LEN 4096
#define MAX_ARGS 128
#define MAX_HISTORY_SIZE 15
#define HISTORY_FILE ".shell_history"

// --- Data Structures for Parsing ---
typedef struct {
    char *name;
    char **args;
    char *input_file;
    char *output_file;
    int append_output;
    int redirect_error; // 0=ok, 1=input error, 2=output error
} AtomicCmd;

typedef struct {
    AtomicCmd *atomics;
    int atomic_count;
} CmdGroup;

typedef struct {
    CmdGroup group;
    int is_background;
} CommandBlock;

typedef struct {
    CommandBlock *blocks;
    int block_count;
} ShellCmd;


// --- Data Structures for Job Control ---
typedef enum { RUNNING, STOPPED } JobState;
typedef struct { pid_t pid; int job_id; char* command; JobState state; } Job;

// --- Global Variables (declared extern) ---
extern char SHELL_HOME[PATH_MAX];
extern char PREV_CWD[PATH_MAX];
extern Job* background_jobs[MAX_ARGS];
extern int job_count;
extern pid_t foreground_pgid;

// --- Function Prototypes ---
void print_prompt();
ShellCmd* parse_input(const char* input);
void free_shell_cmd(ShellCmd* cmd);
void execute_shell_cmd(ShellCmd* cmd);
int is_intrinsic(const char* cmd);
void handle_intrinsic(char** args);
void handle_hop(char** args);
void handle_reveal(char** args);
void handle_log(char** args);
void handle_activities();
void handle_ping(char** args);
void handle_fg(char** args);
void handle_bg(char** args);
void load_history();
void save_history();
void add_to_history(const char* cmd);
void print_history();
void purge_history();
char* get_history_by_index(int index);
void setup_signal_handlers();
void check_background_processes();
void add_job(pid_t pid, const char* command, JobState state);
void remove_job(pid_t pid);

#endif // SHELL_H

// // // #ifndef SHELL_H
// // // #define SHELL_H

// // // #include <stdio.h>
// // // #include <stdlib.h>
// // // #include <string.h>
// // // #include <unistd.h>
// // // #include <sys/wait.h>
// // // #include <sys/types.h>
// // // #include <sys/stat.h>
// // // #include <fcntl.h>
// // // #include <dirent.h>
// // // #include <pwd.h>
// // // #include <limits.h>
// // // #include <signal.h>
// // // #include <errno.h>
// // // #include <ctype.h>

// // // // --- Constants ---
// // // #define MAX_CMD_LEN 4096
// // // #define MAX_ARGS 128
// // // #define MAX_HISTORY_SIZE 15
// // // #define HISTORY_FILE ".shell_history"

// // // // --- Data Structures for Parsing ---
// // // typedef struct {
// // //     char *name;
// // //     char **args;
// // //     int arg_count;
// // //     char *input_file;
// // //     char *output_file;
// // //     int append_output; // Flag for >>
// // // } AtomicCmd;

// // // typedef struct {
// // //     AtomicCmd *atomics;
// // //     int atomic_count;
// // // } CmdGroup;

// // // typedef struct {
// // //     CmdGroup *groups;
// // //     int group_count;
// // //     int background; // Flag for ending with '&'
// // // } ShellCmd;

// // // // --- Data Structures for Job Control ---
// // // typedef enum {
// // //     RUNNING,
// // //     STOPPED
// // // } JobState;

// // // typedef struct {
// // //     pid_t pid;
// // //     int job_id;
// // //     char* command;
// // //     JobState state;
// // // } Job;

// // // // --- Global Variables (declared extern) ---
// // // extern char SHELL_HOME[PATH_MAX];
// // // extern char PREV_CWD[PATH_MAX];
// // // extern Job* background_jobs[MAX_ARGS];
// // // extern int job_count;
// // // extern pid_t foreground_pgid;

// // // // --- Function Prototypes ---

// // // // prompt.c
// // // void print_prompt();

// // // // parser.c
// // // ShellCmd* parse_input(const char* input);
// // // void free_shell_cmd(ShellCmd* cmd);
// // // int is_valid_syntax(const char* input);

// // // // execute.c
// // // void execute_shell_cmd(ShellCmd* cmd);

// // // // intrinsics.c
// // // int is_intrinsic(const char* cmd);
// // // void handle_intrinsic(char** args);
// // // void handle_hop(char** args);
// // // void handle_reveal(char** args);
// // // void handle_log(char** args);
// // // void handle_activities();
// // // void handle_ping(char** args);
// // // void handle_fg(char** args);
// // // void handle_bg(char** args);

// // // // log.c
// // // void load_history();
// // // void save_history();
// // // void add_to_history(const char* cmd);
// // // void print_history();
// // // void purge_history();
// // // char* get_history_by_index(int index);

// // // // signals.c
// // // void setup_signal_handlers();
// // // void check_background_processes();
// // // void add_job(pid_t pid, const char* command, JobState state);
// // // void remove_job(pid_t pid);

// // // #endif // SHELL_H
// // #ifndef SHELL_H
// // #define SHELL_H

// // #include <stdio.h>
// // #include <stdlib.h>
// // #include <string.h>
// // #include <unistd.h>
// // #include <sys/wait.h>
// // #include <sys/types.h>
// // #include <sys/stat.h>
// // #include <fcntl.h>
// // #include <dirent.h>
// // #include <pwd.h>
// // #include <limits.h>
// // #include <signal.h>
// // #include <errno.h>
// // #include <ctype.h>
// // #include <strings.h> // For strcasecmp

// // // --- Constants ---
// // #define MAX_CMD_LEN 4096
// // #define MAX_ARGS 128
// // #define MAX_HISTORY_SIZE 15
// // #define HISTORY_FILE ".shell_history"

// // // --- Data Structures for Parsing ---
// // typedef struct {
// //     char *name;
// //     char **args;
// //     int arg_count;
// //     char *input_file;
// //     char *output_file;
// //     int append_output; // Flag for >>
// // } AtomicCmd;

// // typedef struct {
// //     AtomicCmd *atomics;
// //     int atomic_count;
// // } CmdGroup;

// // // Updated struct to handle sequences of commands separated by ';' or '&'
// // typedef struct {
// //     CmdGroup group;
// //     int is_background;
// // } CommandBlock;

// // typedef struct {
// //     CommandBlock *blocks;
// //     int block_count;
// // } ShellCmd;


// // // --- Data Structures for Job Control ---
// // typedef enum {
// //     RUNNING,
// //     STOPPED
// // } JobState;

// // typedef struct {
// //     pid_t pid;
// //     int job_id;
// //     char* command;
// //     JobState state;
// // } Job;

// // // --- Global Variables (declared extern) ---
// // extern char SHELL_HOME[PATH_MAX];
// // extern char PREV_CWD[PATH_MAX];
// // extern Job* background_jobs[MAX_ARGS];
// // extern int job_count;
// // extern pid_t foreground_pgid;

// // // --- Function Prototypes ---

// // // prompt.c
// // void print_prompt();

// // // parser.c
// // ShellCmd* parse_input(const char* input);
// // void free_shell_cmd(ShellCmd* cmd);
// // int is_valid_syntax(const char* input);

// // // execute.c
// // void execute_shell_cmd(ShellCmd* cmd);

// // // intrinsics.c
// // int is_intrinsic(const char* cmd);
// // void handle_intrinsic(char** args);
// // void handle_hop(char** args);
// // void handle_reveal(char** args);
// // void handle_log(char** args);
// // void handle_activities();
// // void handle_ping(char** args);
// // void handle_fg(char** args);
// // void handle_bg(char** args);

// // // log.c
// // void load_history();
// // void save_history();
// // void add_to_history(const char* cmd);
// // void print_history();
// // void purge_history();
// // char* get_history_by_index(int index);

// // // signals.c
// // void setup_signal_handlers();
// // void check_background_processes();
// // void add_job(pid_t pid, const char* command, JobState state);
// // void remove_job(pid_t pid);

// // #endif // SHELL_H

// #ifndef SHELL_H
// #define SHELL_H

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <dirent.h>
// #include <pwd.h>
// #include <limits.h>
// #include <signal.h>
// #include <errno.h>
// #include <ctype.h>
// #include <strings.h> // For strcasecmp

// // --- Constants ---
// #define MAX_CMD_LEN 4096
// #define MAX_ARGS 128
// #define MAX_HISTORY_SIZE 15
// #define HISTORY_FILE ".shell_history"

// // --- Data Structures for Parsing ---
// typedef struct {
//     char *name;
//     char **args;
//     int arg_count;
//     char *input_file;
//     char *output_file;
//     int append_output; // Flag for >>
// } AtomicCmd;

// typedef struct {
//     AtomicCmd *atomics;
//     int atomic_count;
// } CmdGroup;

// // Represents one command group (pipeline) and whether it runs in the background.
// typedef struct {
//     CmdGroup group;
//     int is_background;
// } CommandBlock;

// // Represents the entire command line as a sequence of blocks.
// typedef struct {
//     CommandBlock *blocks;
//     int block_count;
// } ShellCmd;


// // --- Data Structures for Job Control ---
// typedef enum {
//     RUNNING,
//     STOPPED
// } JobState;

// typedef struct {
//     pid_t pid;
//     int job_id;
//     char* command;
//     JobState state;
// } Job;

// // --- Global Variables (declared extern) ---
// extern char SHELL_HOME[PATH_MAX];
// extern char PREV_CWD[PATH_MAX];
// extern Job* background_jobs[MAX_ARGS];
// extern int job_count;
// extern pid_t foreground_pgid;

// // --- Function Prototypes ---
// void print_prompt();
// ShellCmd* parse_input(const char* input);
// void free_shell_cmd(ShellCmd* cmd);
// void execute_shell_cmd(ShellCmd* cmd);
// int is_intrinsic(const char* cmd);
// void handle_intrinsic(char** args);
// void handle_hop(char** args);
// void handle_reveal(char** args);
// void load_history();
// void save_history();
// void add_to_history(const char* cmd);
// void print_history();
// void purge_history();
// char* get_history_by_index(int index);
// void setup_signal_handlers();
// void check_background_processes();
// void add_job(pid_t pid, const char* command, JobState state);
// void remove_job(pid_t pid);

// #endif // SHELL_H