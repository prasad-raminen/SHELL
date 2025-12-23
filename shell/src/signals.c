#include "shell.h"

void sigint_handler(int sig) {
    (void)sig;
    if (foreground_pgid > 0) kill(-foreground_pgid, SIGINT);
}

void sigtstp_handler(int sig) {
    (void)sig;
    if (foreground_pgid > 0) {
        kill(-foreground_pgid, SIGTSTP);
        add_job(foreground_pgid, "Foreground Process", STOPPED);
        printf("\n[%d] Stopped\n", job_count);
        foreground_pgid = 0;
    }
}

void setup_signal_handlers() {
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);
}

void check_background_processes() {
    int status;
    pid_t pid;
    int job_completed = 0;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        Job* job = NULL;
        for (int i = 0; i < MAX_ARGS; i++) {
            if (background_jobs[i] && background_jobs[i]->pid == pid) {
                job = background_jobs[i];
                break;
            }
        }
        if (!job) continue;

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            printf("\n%s with pid %d exited %s\n", job->command, pid, 
                   WIFEXITED(status) ? "normally" : "abnormally");
            remove_job(pid);
            job_completed = 1;
        } else if (WIFSTOPPED(status)) {
             job->state = STOPPED;
        }
    }
    
    if (job_completed) {
        print_prompt();
    }
}

void add_job(pid_t pid, const char* command, JobState state) {
    job_count++;
    for (int i = 0; i < MAX_ARGS; ++i) {
        if (background_jobs[i] == NULL) {
            background_jobs[i] = malloc(sizeof(Job));
            background_jobs[i]->pid = pid;
            background_jobs[i]->job_id = job_count;
            background_jobs[i]->command = strdup(command);
            background_jobs[i]->state = state;
            return;
        }
    }
}

void remove_job(pid_t pid) {
     for (int i = 0; i < MAX_ARGS; ++i) {
        if (background_jobs[i] != NULL && background_jobs[i]->pid == pid) {
            free(background_jobs[i]->command);
            free(background_jobs[i]);
            background_jobs[i] = NULL;
            return;
        }
    }
}

// #include "shell.h"

// void sigint_handler(int sig) {
//     (void)sig;
//     if (foreground_pgid > 0) kill(-foreground_pgid, SIGINT);
// }

// void sigtstp_handler(int sig) {
//     (void)sig;
//     if (foreground_pgid > 0) {
//         kill(-foreground_pgid, SIGTSTP);
//         add_job(foreground_pgid, "Foreground Process", STOPPED);
//         printf("\n[%d] Stopped\n", job_count);
//         foreground_pgid = 0;
//     }
// }

// void setup_signal_handlers() {
//     signal(SIGINT, sigint_handler);
//     signal(SIGTSTP, sigtstp_handler);
// }

// void check_background_processes() {
//     int status;
//     pid_t pid;
//     int job_completed = 0;
//     while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
//         Job* job = NULL;
//         for (int i = 0; i < MAX_ARGS; i++) {
//             if (background_jobs[i] && background_jobs[i]->pid == pid) {
//                 job = background_jobs[i];
//                 break;
//             }
//         }
//         if (!job) continue;

//         if (WIFEXITED(status) || WIFSIGNALED(status)) {
//             printf("\n%s with pid %d exited %s\n", job->command, pid, 
//                    WIFEXITED(status) ? "normally" : "abnormally");
//             remove_job(pid);
//             job_completed = 1;
//         } else if (WIFSTOPPED(status)) {
//              job->state = STOPPED;
//         }
//     }
    
//     if (job_completed) {
//         print_prompt();
//     }
// }

// void add_job(pid_t pid, const char* command, JobState state) {
//     job_count++;
//     for (int i = 0; i < MAX_ARGS; ++i) {
//         if (background_jobs[i] == NULL) {
//             background_jobs[i] = malloc(sizeof(Job));
//             background_jobs[i]->pid = pid;
//             background_jobs[i]->job_id = job_count;
//             background_jobs[i]->command = strdup(command);
//             background_jobs[i]->state = state;
//             return;
//         }
//     }
// }

// void remove_job(pid_t pid) {
//      for (int i = 0; i < MAX_ARGS; ++i) {
//         if (background_jobs[i] != NULL && background_jobs[i]->pid == pid) {
//             free(background_jobs[i]->command);
//             free(background_jobs[i]);
//             background_jobs[i] = NULL;
//             return;
//         }
//     }
// }
