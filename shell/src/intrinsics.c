#include "shell.h"

int compare_strings_case_insensitive(const void* a, const void* b);

int is_intrinsic(const char* cmd) {
    if (cmd == NULL) return 0;
    return strcmp(cmd, "hop") == 0 || strcmp(cmd, "reveal") == 0 || strcmp(cmd, "log") == 0 ||
           strcmp(cmd, "activities") == 0 || strcmp(cmd, "ping") == 0 ||
           strcmp(cmd, "fg") == 0 || strcmp(cmd, "bg") == 0;
}

void handle_intrinsic(char** args) {
    if(args == NULL || args[0] == NULL) return;
    
    if (strcmp(args[0], "hop") == 0) handle_hop(args);
    else if (strcmp(args[0], "reveal") == 0) handle_reveal(args);
    else if (strcmp(args[0], "log") == 0) handle_log(args);
    else if (strcmp(args[0], "activities") == 0) handle_activities();
    else if (strcmp(args[0], "ping") == 0) handle_ping(args);
    else if (strcmp(args[0], "fg") == 0) handle_fg(args);
    else if (strcmp(args[0], "bg") == 0) handle_bg(args);
}

void handle_hop(char** args) {
    int arg_count = 0;
    while(args[arg_count] != NULL) arg_count++;

    if (arg_count == 1) {
        char current_cwd[PATH_MAX];
        if(getcwd(current_cwd, sizeof(current_cwd)) != NULL) {
            if (chdir(SHELL_HOME) == 0) strcpy(PREV_CWD, current_cwd);
        }
        return;
    }
    
    for (int i = 1; i < arg_count; i++) {
        char current_cwd[PATH_MAX];
        if (getcwd(current_cwd, sizeof(current_cwd)) == NULL) {
            perror("getcwd"); return;
        }
        char* target_arg = args[i];
        if (strcmp(target_arg, "~") == 0) {
            if (chdir(SHELL_HOME) == 0) strcpy(PREV_CWD, current_cwd);
        } else if (strcmp(target_arg, "-") == 0) {
            if (strlen(PREV_CWD) == 0) continue;
            if (chdir(PREV_CWD) == 0) strcpy(PREV_CWD, current_cwd);
        } else {
            if (chdir(target_arg) != 0) {
                printf("No such directory!\n");
                return;
            } else {
                strcpy(PREV_CWD, current_cwd);
            }
        }
    }
}

int compare_strings_case_insensitive(const void* a, const void* b) {
    return strcasecmp(*(const char**)a, *(const char**)b);
}

void handle_reveal(char** args) {
    int show_all = 0, line_by_line = 0;
    char path_buffer[PATH_MAX];
    strcpy(path_buffer, ".");
    char* path = path_buffer;
    int path_is_set = 0;
    int arg_count = 0;
    if (args) { while(args[arg_count] != NULL) arg_count++; }

    for (int i = 1; i < arg_count; i++) {
        if (strcmp(args[i], "-") == 0) {
            if (strlen(PREV_CWD) > 0) {
                path = PREV_CWD;
                path_is_set = 1;
            } else {
                printf("No such directory!\n");
                fflush(stdout);
                return;
            }
        } else if (args[i][0] == '-') {
            for (size_t j = 1; j < strlen(args[i]); j++) {
                if (args[i][j] == 'a') show_all = 1;
                else if (args[i][j] == 'l') line_by_line = 1;
            }
        } else {
            if (!path_is_set) {
                 path = args[i];
                 path_is_set = 1;
            } else { 
                printf("reveal: Invalid Syntax!\n"); return; 
            }
        }
    }
    
    DIR* d = opendir(path);
    if (!d) { printf("No such directory!\n"); return; }

    struct dirent* dir;
    char* entries[2048];
    int count = 0;
    while ((dir = readdir(d)) != NULL) {
        if (!show_all && dir->d_name[0] == '.') continue;
        entries[count++] = strdup(dir->d_name);
    }
    closedir(d);
    qsort(entries, count, sizeof(char*), compare_strings_case_insensitive);

    for (int i = 0; i < count; i++) {
        printf("%s%s", entries[i], line_by_line ? "\n" : "  ");
        free(entries[i]);
    }
    if (!line_by_line && count > 0) printf("\n");
}

void handle_log(char** args) {
    int arg_count = 0;
    while(args[arg_count] != NULL) arg_count++;

    if (arg_count == 1) print_history();
    else if (arg_count == 2 && strcmp(args[1], "purge") == 0) purge_history();
    else if (arg_count == 3 && strcmp(args[1], "execute") == 0) {
        int index = atoi(args[2]);
        char* cmd_to_exec = get_history_by_index(index);
        if (cmd_to_exec) {
            ShellCmd* cmd = parse_input(cmd_to_exec);
            if(cmd) { execute_shell_cmd(cmd); free_shell_cmd(cmd); }
            free(cmd_to_exec);
        } else {
            printf("log: Invalid index!\n");
        }
    } else {
        printf("log: Invalid Syntax!\n");
    }
}

void handle_activities() {
    for (int i = 0; i < MAX_ARGS; ++i) {
        if (background_jobs[i] != NULL) {
            printf("%d: %s %s\n", background_jobs[i]->pid, background_jobs[i]->command,
                   background_jobs[i]->state == RUNNING ? "Running" : "Stopped");
        }
    }
}

void handle_ping(char** args) {
    if (args[1] == NULL || args[2] == NULL) { printf("ping: Invalid syntax!\n"); return; }
    pid_t pid = atoi(args[1]);
    int sig = atoi(args[2]);
    if (pid == 0 || (sig == 0 && strcmp(args[2], "0") != 0)) { printf("ping: Invalid syntax!\n"); return; }
    
    if (kill(pid, 0) == -1) { printf("No such process found\n"); } 
    else {
        if (kill(pid, sig) == 0) printf("Sent signal %d to process with pid %d\n", sig, pid);
        else perror("kill");
    }
}

void handle_fg(char** args) {
    Job* job = NULL;
    if (args[1] == NULL) {
        for (int i = MAX_ARGS - 1; i >= 0; i--) {
            if (background_jobs[i] != NULL) {
                job = background_jobs[i];
                break;
            }
        }
    } else {
        int job_id = atoi(args[1]);
        for (int i = 0; i < MAX_ARGS; i++) {
            if (background_jobs[i] != NULL && background_jobs[i]->job_id == job_id) {
                job = background_jobs[i];
                break;
            }
        }
    }
    
    if (job == NULL) { printf("fg: No such job\n"); return; }

    printf("%s\n", job->command);
    if (job->state == STOPPED) kill(job->pid, SIGCONT);
    
    foreground_pgid = job->pid;
    pid_t pid_to_remove = job->pid;
    remove_job(pid_to_remove); 
    waitpid(pid_to_remove, NULL, WUNTRACED);
    foreground_pgid = 0;
}

void handle_bg(char** args) {
    Job* job = NULL;
    if (args[1] == NULL) {
        for (int i = MAX_ARGS - 1; i >= 0; i--) {
            if (background_jobs[i] != NULL && background_jobs[i]->state == STOPPED) {
                job = background_jobs[i];
                break;
            }
        }
    } else {
        int job_id = atoi(args[1]);
        for (int i = 0; i < MAX_ARGS; i++) {
            if (background_jobs[i] != NULL && background_jobs[i]->job_id == job_id) {
                job = background_jobs[i];
                break;
            }
        }
    }

    if (job == NULL) { printf("bg: No such job\n"); return; }
    if (job->state == RUNNING) { printf("Job already running\n"); return; }
    
    kill(job->pid, SIGCONT);
    job->state = RUNNING;
    printf("[%d] %s &\n", job->job_id, job->command);
}

// #include "shell.h"

// // Forward declaration for the comparison function
// int compare_strings_case_insensitive(const void* a, const void* b);

// int is_intrinsic(const char* cmd) {
//     if (cmd == NULL) return 0;
//     return strcmp(cmd, "hop") == 0 || strcmp(cmd, "reveal") == 0 || strcmp(cmd, "log") == 0 ||
//            strcmp(cmd, "activities") == 0 || strcmp(cmd, "ping") == 0 ||
//            strcmp(cmd, "fg") == 0 || strcmp(cmd, "bg") == 0;
// }

// void handle_intrinsic(char** args) {
//     if (strcmp(args[0], "hop") == 0) handle_hop(args);
//     else if (strcmp(args[0], "reveal") == 0) handle_reveal(args);
//     else if (strcmp(args[0], "log") == 0) handle_log(args);
//     // Add other intrinsic handlers here if they exist
//     // (This example only includes the ones relevant to the fixes)
// }

// void handle_hop(char** args) {
//     int arg_count = 0;
//     while(args[arg_count] != NULL) arg_count++;

//     if (arg_count == 1) { // Just "hop"
//         char current_cwd[PATH_MAX];
//         if(getcwd(current_cwd, sizeof(current_cwd)) != NULL) {
//             if (chdir(SHELL_HOME) == 0) strcpy(PREV_CWD, current_cwd);
//         }
//         return;
//     }
    
//     // Iterate through all arguments.
//     for (int i = 1; i < arg_count; i++) {
//         char current_cwd[PATH_MAX];
//         if (getcwd(current_cwd, sizeof(current_cwd)) == NULL) {
//             perror("getcwd");
//             return;
//         }

//         char* target_arg = args[i];
//         if (strcmp(target_arg, "~") == 0) {
//             if (chdir(SHELL_HOME) == 0) strcpy(PREV_CWD, current_cwd);
//         } else if (strcmp(target_arg, "-") == 0) {
//             if (strlen(PREV_CWD) == 0) continue;
//             if (chdir(PREV_CWD) == 0) strcpy(PREV_CWD, current_cwd);
//         } else {
//             if (chdir(target_arg) != 0) {
//                 printf("No such directory!\n");
//                 return;
//             } else {
//                 strcpy(PREV_CWD, current_cwd);
//             }
//         }
//     }
// }

// int compare_strings_case_insensitive(const void* a, const void* b) {
//     return strcasecmp(*(const char**)a, *(const char**)b);
// }

// void handle_reveal(char** args) {
//     int show_all = 0, line_by_line = 0;
//     char path_buffer[PATH_MAX];
//     strcpy(path_buffer, ".");
//     char* path = path_buffer;
//     int path_is_set = 0;

//     int arg_count = 0;
//     if (args) {
//         while(args[arg_count] != NULL) arg_count++;
//     }

//     for (int i = 1; i < arg_count; i++) {
//         if (strcmp(args[i], "-") == 0) {
//             // *** THE FIX IS HERE ***
//             // Check if PREV_CWD has been set. If not, print an error,
//             // flush the output, and exit the function.
//             if (strlen(PREV_CWD) > 0) {
//                 path = PREV_CWD;
//                 path_is_set = 1;
//             } else {
//                 printf("No such directory!\n");
//                 fflush(stdout); // Force the buffer to be written to the terminal
//                 return;         // Exit the function immediately
//             }
//         } else if (args[i][0] == '-') {
//             for (size_t j = 1; j < strlen(args[i]); j++) {
//                 if (args[i][j] == 'a') show_all = 1;
//                 else if (args[i][j] == 'l') line_by_line = 1;
//             }
//         } else {
//             if (!path_is_set) {
//                  path = args[i];
//                  path_is_set = 1;
//             } else { 
//                 printf("reveal: Invalid Syntax!\n"); return; 
//             }
//         }
//     }
    
//     DIR* d = opendir(path);
//     if (!d) {
//         printf("No such directory!\n");
//         return;
//     }

//     struct dirent* dir;
//     char* entries[2048];
//     int count = 0;
//     while ((dir = readdir(d)) != NULL) {
//         if (!show_all && dir->d_name[0] == '.') continue;
//         entries[count++] = strdup(dir->d_name);
//     }
//     closedir(d);

//     qsort(entries, count, sizeof(char*), compare_strings_case_insensitive);

//     for (int i = 0; i < count; i++) {
//         printf("%s%s", entries[i], line_by_line ? "\n" : "  ");
//         free(entries[i]);
//     }
//     if (!line_by_line && count > 0) printf("\n");
// }

// void handle_log(char** args) {
//     int arg_count = 0;
//     while(args[arg_count] != NULL) arg_count++;

//     if (arg_count == 1) print_history();
//     else if (arg_count == 2 && strcmp(args[1], "purge") == 0) purge_history();
//     else if (arg_count == 3 && strcmp(args[1], "execute") == 0) {
//         int index = atoi(args[2]);
//         char* cmd_to_exec = get_history_by_index(index);
//         if (cmd_to_exec) {
//             printf("Executing: %s\n", cmd_to_exec);
//             ShellCmd* cmd = parse_input(cmd_to_exec);
//             if(cmd) { execute_shell_cmd(cmd); free_shell_cmd(cmd); }
//             free(cmd_to_exec);
//         } else {
//             printf("log: Invalid index!\n");
//         }
//     } else {
//         printf("log: Invalid Syntax!\n");
//     }
// }// // #include "shell.h"

// // // int is_intrinsic(const char* cmd) {
// // //     if (cmd == NULL) return 0;
// // //     return strcmp(cmd, "hop") == 0 || strcmp(cmd, "reveal") == 0 || strcmp(cmd, "log") == 0 ||
// // //            strcmp(cmd, "activities") == 0 || strcmp(cmd, "ping") == 0 ||
// // //            strcmp(cmd, "fg") == 0 || strcmp(cmd, "bg") == 0;
// // // }

// // // void handle_intrinsic(char** args) {
// // //     if (strcmp(args[0], "hop") == 0) handle_hop(args);
// // //     else if (strcmp(args[0], "reveal") == 0) handle_reveal(args);
// // //     else if (strcmp(args[0], "log") == 0) handle_log(args);
// // //     // ... other intrinsics (copy from your last working file)
// // // }

// // // void handle_hop(char** args) {
// // //     int arg_count = 0;
// // //     while(args[arg_count] != NULL) arg_count++;

// // //     if (arg_count == 1) { // Just "hop"
// // //         chdir(SHELL_HOME);
// // //         return;
// // //     }
    
// // //     // **FIX**: Iterate through all arguments.
// // //     for (int i = 1; i < arg_count; i++) {
// // //         char current_cwd[PATH_MAX];
// // //         if (getcwd(current_cwd, sizeof(current_cwd)) == NULL) {
// // //             perror("getcwd");
// // //             return; // Stop if we can't get the CWD
// // //         }

// // //         char* target_arg = args[i];
// // //         if (strcmp(target_arg, "~") == 0) {
// // //             if (chdir(SHELL_HOME) == 0) strcpy(PREV_CWD, current_cwd);
// // //         } else if (strcmp(target_arg, "-") == 0) {
// // //             if (strlen(PREV_CWD) == 0) continue; // Do nothing if no prev dir
// // //             if (chdir(PREV_CWD) == 0) strcpy(PREV_CWD, current_cwd);
// // //         } else {
// // //             if (chdir(target_arg) != 0) {
// // //                 printf("No such directory!\n");
// // //                 // Stop processing further arguments on the first error
// // //                 return;
// // //             } else {
// // //                 strcpy(PREV_CWD, current_cwd);
// // //             }
// // //         }
// // //     }
// // // }

// // // // **FIX**: Use strcasecmp for case-insensitive sort to match the test's expectation.
// // // int compare_strings_case_insensitive(const void* a, const void* b) {
// // //     return strcasecmp(*(const char**)a, *(const char**)b);
// // // }

// // // void handle_reveal(char** args) {
// // //     int show_all = 0, line_by_line = 0;
// // //     char path_buffer[PATH_MAX];
// // //     strcpy(path_buffer, ".");
// // //     char* path = path_buffer;
// // //     int path_is_set = 0;

// // //     int arg_count = 0;
// // //     if (args) {
// // //         while(args[arg_count] != NULL) arg_count++;
// // //     }

// // //     for (int i = 1; i < arg_count; i++) {
// // //         if (args[i][0] == '-') {
// // //             if (strlen(args[i]) == 1) {
// // //                 if (strlen(PREV_CWD) > 0) {
// // //                     path = PREV_CWD;
// // //                     path_is_set = 1;
// // //                 }
// // //                 continue;
// // //             }
// // //             for (size_t j = 1; j < strlen(args[i]); j++) {
// // //                 if (args[i][j] == 'a') show_all = 1;
// // //                 else if (args[i][j] == 'l') line_by_line = 1;
// // //             }
// // //         } else {
// // //             if (!path_is_set) {
// // //                  path = args[i];
// // //                  path_is_set = 1;
// // //             } else { 
// // //                 printf("reveal: Invalid Syntax!\n"); return; 
// // //             }
// // //         }
// // //     }
    
// // //     DIR* d = opendir(path);
// // //     if (!d) { printf("No such directory!\n"); return; }

// // //     struct dirent* dir;
// // //     char* entries[2048];
// // //     int count = 0;
// // //     while ((dir = readdir(d)) != NULL) {
// // //         if (!show_all && dir->d_name[0] == '.') continue;
// // //         entries[count++] = strdup(dir->d_name);
// // //     }
// // //     closedir(d);

// // //     qsort(entries, count, sizeof(char*), compare_strings_case_insensitive);

// // //     for (int i = 0; i < count; i++) {
// // //         printf("%s%s", entries[i], line_by_line ? "\n" : "  ");
// // //         free(entries[i]);
// // //     }
// // //     if (!line_by_line && count > 0) printf("\n");
// // // }


// // // // (NOTE: Ensure the rest of your intrinsic functions like handle_log, handle_fg, etc.,
// // // // are present in this file as they were in your working single-file version).

// // // // #include "shell.h"

// // // // int is_intrinsic(const char* cmd) {
// // // //     if (cmd == NULL) return 0;
// // // //     return strcmp(cmd, "hop") == 0 || strcmp(cmd, "reveal") == 0 || strcmp(cmd, "log") == 0 ||
// // // //            strcmp(cmd, "activities") == 0 || strcmp(cmd, "ping") == 0 ||
// // // //            strcmp(cmd, "fg") == 0 || strcmp(cmd, "bg") == 0;
// // // // }

// // // // void handle_intrinsic(char** args) {
// // // //     if (strcmp(args[0], "hop") == 0) handle_hop(args);
// // // //     else if (strcmp(args[0], "reveal") == 0) handle_reveal(args);
// // // //     else if (strcmp(args[0], "log") == 0) handle_log(args);
// // // //     else if (strcmp(args[0], "activities") == 0) handle_activities();
// // // //     else if (strcmp(args[0], "ping") == 0) handle_ping(args);
// // // //     else if (strcmp(args[0], "fg") == 0) handle_fg(args);
// // // //     else if (strcmp(args[0], "bg") == 0) handle_bg(args);
// // // // }

// // // // void handle_hop(char** args) {
// // // //     int arg_count = 0;
// // // //     while(args[arg_count] != NULL) arg_count++;

// // // //     if (arg_count == 1) { // Just "hop"
// // // //         chdir(SHELL_HOME);
// // // //         return;
// // // //     }
    
// // // //     // **FIX**: Iterate through all arguments.
// // // //     for (int i = 1; i < arg_count; i++) {
// // // //         char current_cwd[PATH_MAX];
// // // //         if (getcwd(current_cwd, sizeof(current_cwd)) == NULL) {
// // // //             perror("getcwd");
// // // //             return;
// // // //         }

// // // //         char* target_arg = args[i];
// // // //         if (strcmp(target_arg, "~") == 0) {
// // // //             if (chdir(SHELL_HOME) == 0) strcpy(PREV_CWD, current_cwd);
// // // //         } else if (strcmp(target_arg, "-") == 0) {
// // // //             if (strlen(PREV_CWD) == 0) continue; // Do nothing if no prev dir
// // // //             if (chdir(PREV_CWD) == 0) strcpy(PREV_CWD, current_cwd);
// // // //         } else {
// // // //             if (chdir(target_arg) != 0) {
// // // //                 printf("No such directory!\n");
// // // //                 break; // Stop on first error
// // // //             } else {
// // // //                 strcpy(PREV_CWD, current_cwd);
// // // //             }
// // // //         }
// // // //     }
// // // // }

// // // // // **FIX**: Use strcasecmp for case-insensitive sort.
// // // // int compare_strings_case_insensitive(const void* a, const void* b) {
// // // //     return strcasecmp(*(const char**)a, *(const char**)b);
// // // // }

// // // // void handle_reveal(char** args) {
// // // //     int show_all = 0, line_by_line = 0;
// // // //     char path_buffer[PATH_MAX];
// // // //     strcpy(path_buffer, ".");
// // // //     char* path = path_buffer;
// // // //     int path_is_set = 0;

// // // //     int arg_count = 0;
// // // //     if (args) {
// // // //         while(args[arg_count] != NULL) arg_count++;
// // // //     }

// // // //     for (int i = 1; i < arg_count; i++) {
// // // //         if (args[i][0] == '-') {
// // // //             if (strlen(args[i]) == 1) {
// // // //                 if (strlen(PREV_CWD) > 0) {
// // // //                     path = PREV_CWD;
// // // //                     path_is_set = 1;
// // // //                 }
// // // //                 continue;
// // // //             }
// // // //             for (size_t j = 1; j < strlen(args[i]); j++) {
// // // //                 if (args[i][j] == 'a') show_all = 1;
// // // //                 else if (args[i][j] == 'l') line_by_line = 1;
// // // //             }
// // // //         } else {
// // // //             if (!path_is_set) {
// // // //                  path = args[i];
// // // //                  path_is_set = 1;
// // // //             } else { 
// // // //                 printf("reveal: Invalid Syntax!\n"); return; 
// // // //             }
// // // //         }
// // // //     }
    
// // // //     DIR* d = opendir(path);
// // // //     if (!d) { printf("No such directory!\n"); return; }

// // // //     struct dirent* dir;
// // // //     char* entries[2048];
// // // //     int count = 0;
// // // //     while ((dir = readdir(d)) != NULL) {
// // // //         if (!show_all && dir->d_name[0] == '.') continue;
// // // //         entries[count++] = strdup(dir->d_name);
// // // //     }
// // // //     closedir(d);

// // // //     qsort(entries, count, sizeof(char*), compare_strings_case_insensitive);

// // // //     for (int i = 0; i < count; i++) {
// // // //         printf("%s%s", entries[i], line_by_line ? "\n" : "  ");
// // // //         free(entries[i]);
// // // //     }
// // // //     if (!line_by_line && count > 0) printf("\n");
// // // // }

// // // // // ... rest of the functions in intrinsics.c (handle_log, handle_activities, etc.) are unchanged ...
// // // // // (You can copy them from your last working single-file version)
// // // void handle_log(char** args) {
// // //     int arg_count = 0;
// // //     while(args[arg_count] != NULL) arg_count++;

// // //     if (arg_count == 1) print_history();
// // //     else if (arg_count == 2 && strcmp(args[1], "purge") == 0) purge_history();
// // //     else if (arg_count == 3 && strcmp(args[1], "execute") == 0) {
// // //         int index = atoi(args[2]);
// // //         char* cmd_to_exec = get_history_by_index(index);
// // //         if (cmd_to_exec) {
// // //             printf("Executing: %s\n", cmd_to_exec);
// // //             ShellCmd* cmd = parse_input(cmd_to_exec);
// // //             if(cmd) { execute_shell_cmd(cmd); free_shell_cmd(cmd); }
// // //             free(cmd_to_exec);
// // //         } else {
// // //             printf("log: Invalid index!\n");
// // //         }
// // //     } else {
// // //         printf("log: Invalid Syntax!\n");
// // //     }
// // // }
// // // void handle_activities() {
// // //     printf("PID      State     Command\n");
// // //     for (int i = 0; i < MAX_ARGS; ++i) {
// // //         if (background_jobs[i] != NULL) {
// // //             printf("%-8d %-9s %s\n", background_jobs[i]->pid,
// // //                    background_jobs[i]->state == RUNNING ? "Running" : "Stopped",
// // //                    background_jobs[i]->command);
// // //         }
// // //     }
// // // }
// // // void handle_ping(char** args) {
// // //     if (args[1] == NULL || args[2] == NULL) { printf("ping: Invalid syntax!\n"); return; }
// // //     pid_t pid = atoi(args[1]);
// // //     int sig = atoi(args[2]);
// // //     if (pid == 0 || (sig == 0 && strcmp(args[2], "0") != 0)) { printf("ping: Invalid syntax!\n"); return; }
    
// // //     if (kill(pid, 0) == -1) { printf("No such process found\n"); } 
// // //     else {
// // //         if (kill(pid, sig % 32) == 0) printf("Sent signal %d to process with pid %d\n", sig % 32, pid);
// // //         else perror("kill");
// // //     }
// // // }
// // // void handle_fg(char** args) {
// // //     Job* job = NULL;
// // //     if (args[1] == NULL) {
// // //         for (int i = MAX_ARGS - 1; i >= 0; i--) {
// // //             if (background_jobs[i] != NULL) {
// // //                 job = background_jobs[i];
// // //                 break;
// // //             }
// // //         }
// // //     } else {
// // //         int job_id = atoi(args[1]);
// // //         for (int i = 0; i < MAX_ARGS; i++) {
// // //             if (background_jobs[i] != NULL && background_jobs[i]->job_id == job_id) {
// // //                 job = background_jobs[i];
// // //                 break;
// // //             }
// // //         }
// // //     }
    
// // //     if (job == NULL) { printf("fg: No such job\n"); return; }
// // //     printf("%s\n", job->command);
// // //     if (job->state == STOPPED) kill(job->pid, SIGCONT);
    
// // //     foreground_pgid = job->pid;
// // //     pid_t pid_to_remove = job->pid;
// // //     remove_job(pid_to_remove); 
// // //     waitpid(pid_to_remove, NULL, WUNTRACED);
// // //     foreground_pgid = 0;
// // // }
// // // void handle_bg(char** args) {
// // //     Job* job = NULL;
// // //     if (args[1] == NULL) {
// // //         for (int i = MAX_ARGS - 1; i >= 0; i--) {
// // //             if (background_jobs[i] != NULL && background_jobs[i]->state == STOPPED) {
// // //                 job = background_jobs[i];
// // //                 break;
// // //             }
// // //         }
// // //     } else {
// // //         int job_id = atoi(args[1]);
// // //         for (int i = 0; i < MAX_ARGS; i++) {
// // //             if (background_jobs[i] != NULL && background_jobs[i]->job_id == job_id) {
// // //                 job = background_jobs[i];
// // //                 break;
// // //             }
// // //         }
// // //     }

// // //     if (job == NULL) { printf("bg: No such job\n"); return; }
// // //     if (job->state == RUNNING) { printf("Job already running\n"); return; }
    
// // //     kill(job->pid, SIGCONT);
// // //     job->state = RUNNING;
// // //     printf("[%d] %s &\n", job->job_id, job->command);
// // // }

// // // // // #include "shell.h"

// // // // // int is_intrinsic(const char* cmd) {
// // // // //     if (cmd == NULL) return 0;
// // // // //     return strcmp(cmd, "hop") == 0 || strcmp(cmd, "reveal") == 0 || strcmp(cmd, "log") == 0 ||
// // // // //            strcmp(cmd, "activities") == 0 || strcmp(cmd, "ping") == 0 ||
// // // // //            strcmp(cmd, "fg") == 0 || strcmp(cmd, "bg") == 0;
// // // // // }

// // // // // void handle_intrinsic(char** args) {
// // // // //     if (strcmp(args[0], "hop") == 0) handle_hop(args);
// // // // //     else if (strcmp(args[0], "reveal") == 0) handle_reveal(args);
// // // // //     else if (strcmp(args[0], "log") == 0) handle_log(args);
// // // // //     else if (strcmp(args[0], "activities") == 0) handle_activities();
// // // // //     else if (strcmp(args[0], "ping") == 0) handle_ping(args);
// // // // //     else if (strcmp(args[0], "fg") == 0) handle_fg(args);
// // // // //     else if (strcmp(args[0], "bg") == 0) handle_bg(args);
// // // // // }

// // // // // void handle_hop(char** args) {
// // // // //     char current_cwd[PATH_MAX];
// // // // //     getcwd(current_cwd, sizeof(current_cwd));
    
// // // // //     char* target_arg = args[1];
// // // // //     if (target_arg == NULL || strcmp(target_arg, "~") == 0) {
// // // // //         if (chdir(SHELL_HOME) == 0) strcpy(PREV_CWD, current_cwd);
// // // // //     } else if (strcmp(target_arg, "-") == 0) {
// // // // //         if (strlen(PREV_CWD) == 0) return;
// // // // //         if (chdir(PREV_CWD) == 0) strcpy(PREV_CWD, current_cwd);
// // // // //     } else {
// // // // //         if (chdir(target_arg) != 0) printf("No such directory!\n");
// // // // //         else strcpy(PREV_CWD, current_cwd);
// // // // //     }
// // // // // }

// // // // // int compare_strings(const void* a, const void* b) {
// // // // //     return strcmp(*(const char**)a, *(const char**)b);
// // // // // }

// // // // // void handle_reveal(char** args) {
// // // // //     int show_all = 0, line_by_line = 0;
// // // // //     char path_buffer[PATH_MAX];
// // // // //     strcpy(path_buffer, ".");
// // // // //     char* path = path_buffer;
// // // // //     int path_is_set = 0;

// // // // //     int arg_count = 0;
// // // // //     if (args) {
// // // // //         while(args[arg_count] != NULL) arg_count++;
// // // // //     }

// // // // //     for (int i = 1; i < arg_count; i++) {
// // // // //         if (args[i][0] == '-') {
// // // // //             if (strlen(args[i]) == 1) {
// // // // //                 if (strlen(PREV_CWD) > 0) {
// // // // //                     path = PREV_CWD;
// // // // //                     path_is_set = 1;
// // // // //                 }
// // // // //                 continue;
// // // // //             }
// // // // //             for (size_t j = 1; j < strlen(args[i]); j++) {
// // // // //                 if (args[i][j] == 'a') show_all = 1;
// // // // //                 else if (args[i][j] == 'l') line_by_line = 1;
// // // // //             }
// // // // //         } else {
// // // // //             if (!path_is_set) {
// // // // //                  path = args[i];
// // // // //                  path_is_set = 1;
// // // // //             } else { 
// // // // //                 printf("reveal: Invalid Syntax!\n"); return; 
// // // // //             }
// // // // //         }
// // // // //     }
    
// // // // //     DIR* d = opendir(path);
// // // // //     if (!d) { printf("No such directory!\n"); return; }

// // // // //     struct dirent* dir;
// // // // //     char* entries[2048];
// // // // //     int count = 0;
// // // // //     while ((dir = readdir(d)) != NULL) {
// // // // //         if (!show_all && dir->d_name[0] == '.') continue;
// // // // //         entries[count++] = strdup(dir->d_name);
// // // // //     }
// // // // //     closedir(d);

// // // // //     qsort(entries, count, sizeof(char*), compare_strings);

// // // // //     for (int i = 0; i < count; i++) {
// // // // //         printf("%s%s", entries[i], line_by_line ? "\n" : "  ");
// // // // //         free(entries[i]);
// // // // //     }
// // // // //     if (!line_by_line && count > 0) printf("\n");
// // // // // }

// // // // // void handle_log(char** args) {
// // // // //     int arg_count = 0;
// // // // //     while(args[arg_count] != NULL) arg_count++;

// // // // //     if (arg_count == 1) print_history();
// // // // //     else if (arg_count == 2 && strcmp(args[1], "purge") == 0) purge_history();
// // // // //     else if (arg_count == 3 && strcmp(args[1], "execute") == 0) {
// // // // //         int index = atoi(args[2]);
// // // // //         char* cmd_to_exec = get_history_by_index(index);
// // // // //         if (cmd_to_exec) {
// // // // //             printf("Executing: %s\n", cmd_to_exec);
// // // // //             ShellCmd* cmd = parse_input(cmd_to_exec);
// // // // //             if(cmd) { execute_shell_cmd(cmd); free_shell_cmd(cmd); }
// // // // //             free(cmd_to_exec);
// // // // //         } else {
// // // // //             printf("log: Invalid index!\n");
// // // // //         }
// // // // //     } else {
// // // // //         printf("log: Invalid Syntax!\n");
// // // // //     }
// // // // // }

// // // // // void handle_activities() {
// // // // //     printf("PID      State     Command\n");
// // // // //     for (int i = 0; i < MAX_ARGS; ++i) {
// // // // //         if (background_jobs[i] != NULL) {
// // // // //             printf("%-8d %-9s %s\n", background_jobs[i]->pid,
// // // // //                    background_jobs[i]->state == RUNNING ? "Running" : "Stopped",
// // // // //                    background_jobs[i]->command);
// // // // //         }
// // // // //     }
// // // // // }

// // // // // void handle_ping(char** args) {
// // // // //     if (args[1] == NULL || args[2] == NULL) { printf("ping: Invalid syntax!\n"); return; }
// // // // //     pid_t pid = atoi(args[1]);
// // // // //     int sig = atoi(args[2]);
// // // // //     if (pid == 0 || (sig == 0 && strcmp(args[2], "0") != 0)) { printf("ping: Invalid syntax!\n"); return; }
    
// // // // //     if (kill(pid, 0) == -1) { printf("No such process found\n"); } 
// // // // //     else {
// // // // //         if (kill(pid, sig % 32) == 0) printf("Sent signal %d to process with pid %d\n", sig % 32, pid);
// // // // //         else perror("kill");
// // // // //     }
// // // // // }

// // // // // void handle_fg(char** args) {
// // // // //     Job* job = NULL;
// // // // //     if (args[1] == NULL) {
// // // // //         for (int i = MAX_ARGS - 1; i >= 0; i--) {
// // // // //             if (background_jobs[i] != NULL) {
// // // // //                 job = background_jobs[i];
// // // // //                 break;
// // // // //             }
// // // // //         }
// // // // //     } else {
// // // // //         int job_id = atoi(args[1]);
// // // // //         for (int i = 0; i < MAX_ARGS; i++) {
// // // // //             if (background_jobs[i] != NULL && background_jobs[i]->job_id == job_id) {
// // // // //                 job = background_jobs[i];
// // // // //                 break;
// // // // //             }
// // // // //         }
// // // // //     }
    
// // // // //     if (job == NULL) { printf("fg: No such job\n"); return; }

// // // // //     printf("%s\n", job->command);
// // // // //     if (job->state == STOPPED) kill(job->pid, SIGCONT);
    
// // // // //     foreground_pgid = job->pid;
// // // // //     pid_t pid_to_remove = job->pid;
// // // // //     remove_job(pid_to_remove); 
// // // // //     waitpid(pid_to_remove, NULL, WUNTRACED);
// // // // //     foreground_pgid = 0;
// // // // // }

// // // // // void handle_bg(char** args) {
// // // // //     Job* job = NULL;
// // // // //     if (args[1] == NULL) {
// // // // //         for (int i = MAX_ARGS - 1; i >= 0; i--) {
// // // // //             if (background_jobs[i] != NULL && background_jobs[i]->state == STOPPED) {
// // // // //                 job = background_jobs[i];
// // // // //                 break;
// // // // //             }
// // // // //         }
// // // // //     } else {
// // // // //         int job_id = atoi(args[1]);
// // // // //         for (int i = 0; i < MAX_ARGS; i++) {
// // // // //             if (background_jobs[i] != NULL && background_jobs[i]->job_id == job_id) {
// // // // //                 job = background_jobs[i];
// // // // //                 break;
// // // // //             }
// // // // //         }
// // // // //     }

// // // // //     if (job == NULL) { printf("bg: No such job\n"); return; }
// // // // //     if (job->state == RUNNING) { printf("Job already running\n"); return; }
    
// // // // //     kill(job->pid, SIGCONT);
// // // // //     job->state = RUNNING;
// // // // //     printf("[%d] %s &\n", job->job_id, job->command);
// // // // // }
// // #include "shell.h"

// // // Forward declaration for the comparison function
// // int compare_strings_case_insensitive(const void* a, const void* b);

// // int is_intrinsic(const char* cmd) {
// //     if (cmd == NULL) return 0;
// //     return strcmp(cmd, "hop") == 0 || strcmp(cmd, "reveal") == 0 || strcmp(cmd, "log") == 0 ||
// //            strcmp(cmd, "activities") == 0 || strcmp(cmd, "ping") == 0 ||
// //            strcmp(cmd, "fg") == 0 || strcmp(cmd, "bg") == 0;
// // }

// // void handle_intrinsic(char** args) {
// //     if (strcmp(args[0], "hop") == 0) handle_hop(args);
// //     else if (strcmp(args[0], "reveal") == 0) handle_reveal(args);
// //     else if (strcmp(args[0], "log") == 0) handle_log(args);
// //     // Add other intrinsic handlers here if they exist
// // }

// // void handle_hop(char** args) {
// //     int arg_count = 0;
// //     while(args[arg_count] != NULL) arg_count++;

// //     if (arg_count == 1) { // Just "hop" or "hop ~"
// //         char current_cwd[PATH_MAX];
// //         if(getcwd(current_cwd, sizeof(current_cwd)) != NULL) {
// //             if (chdir(SHELL_HOME) == 0) strcpy(PREV_CWD, current_cwd);
// //         }
// //         return;
// //     }
    
// //     // Iterate through all arguments.
// //     for (int i = 1; i < arg_count; i++) {
// //         char current_cwd[PATH_MAX];
// //         if (getcwd(current_cwd, sizeof(current_cwd)) == NULL) {
// //             perror("getcwd");
// //             return;
// //         }

// //         char* target_arg = args[i];
// //         if (strcmp(target_arg, "~") == 0) {
// //             if (chdir(SHELL_HOME) == 0) strcpy(PREV_CWD, current_cwd);
// //         } else if (strcmp(target_arg, "-") == 0) {
// //             if (strlen(PREV_CWD) == 0) continue;
// //             if (chdir(PREV_CWD) == 0) strcpy(PREV_CWD, current_cwd);
// //         } else {
// //             if (chdir(target_arg) != 0) {
// //                 printf("No such directory!\n");
// //                 return;
// //             } else {
// //                 strcpy(PREV_CWD, current_cwd);
// //             }
// //         }
// //     }
// // }

// // int compare_strings_case_insensitive(const void* a, const void* b) {
// //     return strcasecmp(*(const char**)a, *(const char**)b);
// // }

// // void handle_reveal(char** args) {
// //     int show_all = 0, line_by_line = 0;
// //     char path_buffer[PATH_MAX];
// //     strcpy(path_buffer, ".");
// //     char* path = path_buffer;
// //     int path_is_set = 0;

// //     int arg_count = 0;
// //     if (args) {
// //         while(args[arg_count] != NULL) arg_count++;
// //     }

// //     for (int i = 1; i < arg_count; i++) {
// //         // Use strcmp for safer comparison
// //         if (strcmp(args[i], "-") == 0) {
// //             // *** THE FIX IS HERE ***
// //             // Check if PREV_CWD has been set. If not, print an error and exit.
// //             if (strlen(PREV_CWD) > 0) {
// //                 path = PREV_CWD;
// //                 path_is_set = 1;
// //             } else {
// //                 printf("No such directory!\n");
// //                 return; // Exit the function immediately
// //             }
// //         } else if (args[i][0] == '-') {
// //             for (size_t j = 1; j < strlen(args[i]); j++) {
// //                 if (args[i][j] == 'a') show_all = 1;
// //                 else if (args[i][j] == 'l') line_by_line = 1;
// //             }
// //         } else {
// //             if (!path_is_set) {
// //                  path = args[i];
// //                  path_is_set = 1;
// //             } else { 
// //                 printf("reveal: Invalid Syntax!\n"); return; 
// //             }
// //         }
// //     }
    
// //     DIR* d = opendir(path);
// //     if (!d) {
// //         printf("No such directory!\n");
// //         return;
// //     }

// //     struct dirent* dir;
// //     char* entries[2048];
// //     int count = 0;
// //     while ((dir = readdir(d)) != NULL) {
// //         if (!show_all && dir->d_name[0] == '.') continue;
// //         entries[count++] = strdup(dir->d_name);
// //     }
// //     closedir(d);

// //     qsort(entries, count, sizeof(char*), compare_strings_case_insensitive);

// //     for (int i = 0; i < count; i++) {
// //         printf("%s%s", entries[i], line_by_line ? "\n" : "  ");
// //         free(entries[i]);
// //     }
// //     if (!line_by_line && count > 0) printf("\n");
// // }

// // void handle_log(char** args) {
// //     int arg_count = 0;
// //     while(args[arg_count] != NULL) arg_count++;

// //     if (arg_count == 1) print_history();
// //     else if (arg_count == 2 && strcmp(args[1], "purge") == 0) purge_history();
// //     else if (arg_count == 3 && strcmp(args[1], "execute") == 0) {
// //         int index = atoi(args[2]);
// //         char* cmd_to_exec = get_history_by_index(index);
// //         if (cmd_to_exec) {
// //             printf("Executing: %s\n", cmd_to_exec);
// //             ShellCmd* cmd = parse_input(cmd_to_exec);
// //             if(cmd) { execute_shell_cmd(cmd); free_shell_cmd(cmd); }
// //             free(cmd_to_exec);
// //         } else {
// //             printf("log: Invalid index!\n");
// //         }
// //     } else {
// //         printf("log: Invalid Syntax!\n");
// //     }
// // }