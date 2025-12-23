#include "shell.h"

// --- Global Variable Definitions ---
char SHELL_HOME[PATH_MAX];
char PREV_CWD[PATH_MAX] = {0};
Job* background_jobs[MAX_ARGS] = {NULL};
int job_count = 0;
pid_t foreground_pgid = 0;


void initialize_shell() {
    if (getcwd(SHELL_HOME, sizeof(SHELL_HOME)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    if (getcwd(PREV_CWD, sizeof(PREV_CWD)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    load_history();
    setup_signal_handlers();
}

int main() {
    initialize_shell();
    char* input = NULL;
    size_t len = 0;

    while (1) {
        // First check for finished jobs from the previous cycle
        check_background_processes();
        print_prompt();

        errno = 0;
        ssize_t nread = getline(&input, &len, stdin);

        if (nread == -1) {
            if (feof(stdin)) { // Ctrl-D
                printf("logout\n");
                break;
            }
            if (errno == EINTR) { // Interrupted by a signal
                clearerr(stdin);
                printf("\n");
                continue;
            }
            perror("getline");
            break;
        }

        if (nread > 0 && input[nread - 1] == '\n') {
            input[nread - 1] = '\0';
        }
        
        char* trimmed_input = input;
        while (isspace((unsigned char)*trimmed_input)) {
            trimmed_input++;
        }
        if (strlen(trimmed_input) == 0) {
            continue;
        }

        char temp_input[MAX_CMD_LEN];
        strncpy(temp_input, input, sizeof(temp_input) - 1);
        temp_input[sizeof(temp_input) - 1] = '\0';
        char* first_word = strtok(temp_input, " \t\n\r");
        if (first_word != NULL && strcmp(first_word, "log") != 0) {
            add_to_history(input);
        }

        ShellCmd* cmd = parse_input(input);
        if (cmd) {
            execute_shell_cmd(cmd);
            free_shell_cmd(cmd);
        }
    }
    
    for (int i = 0; i < MAX_ARGS; i++) {
        if (background_jobs[i] != NULL) {
            kill(background_jobs[i]->pid, SIGKILL);
        }
    }
    free(input);
    save_history();
    return 0;
}

// #include "shell.h"

// // --- Global Variable Definitions ---
// char SHELL_HOME[PATH_MAX];
// char PREV_CWD[PATH_MAX] = {0};
// Job* background_jobs[MAX_ARGS] = {NULL};
// int job_count = 0;
// pid_t foreground_pgid = 0;


// void initialize_shell() {
//     if (getcwd(SHELL_HOME, sizeof(SHELL_HOME)) == NULL) {
//         perror("getcwd");
//         exit(EXIT_FAILURE);
//     }
//     if (getcwd(PREV_CWD, sizeof(PREV_CWD)) == NULL) {
//         perror("getcwd");
//         exit(EXIT_FAILURE);
//     }
//     load_history();
//     setup_signal_handlers();
// }

// int main() {
//     initialize_shell();
//     char* input = NULL;
//     size_t len = 0;

//     while (1) {
//         check_background_processes();
//         print_prompt();

//         errno = 0;
//         ssize_t nread = getline(&input, &len, stdin);

//         if (nread == -1) {
//             if (feof(stdin)) { // Ctrl-D
//                 printf("logout\n");
//                 break;
//             }
//             if (errno == EINTR) { // Interrupted by a signal
//                 clearerr(stdin);
//                 printf("\n");
//                 continue;
//             }
//             perror("getline");
//             break;
//         }

//         if (nread > 0 && input[nread - 1] == '\n') {
//             input[nread - 1] = '\0';
//         }
        
//         char* trimmed_input = input;
//         while (isspace((unsigned char)*trimmed_input)) {
//             trimmed_input++;
//         }
//         if (strlen(trimmed_input) == 0) {
//             continue;
//         }

//         if (!is_valid_syntax(input)) {
//             printf("Invalid Syntax!\n");
//             continue;
//         }

//         char temp_input[MAX_CMD_LEN];
//         strncpy(temp_input, input, sizeof(temp_input) - 1);
//         temp_input[sizeof(temp_input) - 1] = '\0';
//         char* first_word = strtok(temp_input, " \t\n\r");
//         if (first_word != NULL && strcmp(first_word, "log") != 0) {
//             add_to_history(input);
//         }

//         ShellCmd* cmd = parse_input(input);
//         if (cmd) {
//             execute_shell_cmd(cmd);
//             free_shell_cmd(cmd);
//         }
//     }
    
//     // Clean up remaining jobs on exit
//     for (int i = 0; i < MAX_ARGS; i++) {
//         if (background_jobs[i] != NULL) {
//             kill(background_jobs[i]->pid, SIGKILL);
//         }
//     }
//     free(input);
//     save_history();
//     return 0;
// }
