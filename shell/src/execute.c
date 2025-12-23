#include "shell.h"

void execute_cmd_group(CmdGroup* group, int is_background);
void execute_atomic_cmd(AtomicCmd* atomic, int is_background);

void execute_shell_cmd(ShellCmd* cmd) {
    if (cmd->block_count == 0) return;
    for (int i = 0; i < cmd->block_count; i++) {
        CommandBlock* block = &cmd->blocks[i];
        if (block->group.atomic_count > 0) {
            execute_cmd_group(&block->group, block->is_background);
        }
    }
}

void execute_cmd_group(CmdGroup* group, int is_background) {
    if (group->atomic_count == 0) return;
    if (group->atomic_count == 1) {
        execute_atomic_cmd(&group->atomics[0], is_background);
        return;
    }

    int num_pipes = group->atomic_count - 1;
    int pipe_fds[2];
    int in_fd = STDIN_FILENO;
    pid_t pids[group->atomic_count];

    for (int i = 0; i < group->atomic_count; i++) {
        if (i < num_pipes) pipe(pipe_fds);
        
        pid_t pid = fork();
        pids[i] = pid;
        if (pid == 0) { // Child
            setpgid(0, getpid());
            if (i > 0) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
            if (i < num_pipes) { dup2(pipe_fds[1], STDOUT_FILENO); close(pipe_fds[0]); close(pipe_fds[1]); }

            AtomicCmd* atomic = &group->atomics[i];
            if (atomic->redirect_error) exit(EXIT_FAILURE);

            if (i == 0 && atomic->input_file) {
                 int fd = open(atomic->input_file, O_RDONLY);
                 if(fd < 0) { perror(atomic->input_file); exit(EXIT_FAILURE); }
                 dup2(fd, STDIN_FILENO); close(fd);
            }
            if (i == num_pipes && atomic->output_file) {
                 int flags = O_WRONLY | O_CREAT | (atomic->append_output ? O_APPEND : O_TRUNC);
                 int fd = open(atomic->output_file, flags, 0644);
                 if(fd < 0) { perror(atomic->output_file); exit(EXIT_FAILURE); }
                 dup2(fd, STDOUT_FILENO); close(fd);
            }
            
            if (is_intrinsic(atomic->name)) {
                handle_intrinsic(atomic->args);
                exit(EXIT_SUCCESS);
            } else {
                execvp(atomic->name, atomic->args);
                fprintf(stderr, "%s: Command not found!\n", atomic->name);
                exit(EXIT_FAILURE);
            }
        }
        if (i > 0) close(in_fd);
        if (i < num_pipes) { in_fd = pipe_fds[0]; close(pipe_fds[1]); }
    }

    if (!is_background) {
        foreground_pgid = pids[0];
        for (int i = 0; i < group->atomic_count; i++) waitpid(pids[i], NULL, WUNTRACED);
        foreground_pgid = 0;
    } else {
        char cmd_str[MAX_CMD_LEN] = {0};
        strcpy(cmd_str, group->atomics[0].name); strcat(cmd_str, " | ...");
        add_job(pids[0], cmd_str, RUNNING);
        printf("[%d] %d\n", job_count, pids[0]);
    }
}

void execute_atomic_cmd(AtomicCmd* atomic, int is_background) {
    if (atomic->name == NULL) return;

    if (atomic->redirect_error) {
        if (atomic->redirect_error == 1) {
            fprintf(stderr, "No such file or directory!\n");
        } else if (atomic->redirect_error == 2) {
            fprintf(stderr, "Unable to create file for writing\n");
        }
        return;
    }
    
    if (is_intrinsic(atomic->name)) {
        int stdin_backup = dup(STDIN_FILENO), stdout_backup = dup(STDOUT_FILENO);
        if (atomic->input_file) {
            int fd = open(atomic->input_file, O_RDONLY);
            if (fd < 0) { perror(atomic->input_file); return; }
            dup2(fd, STDIN_FILENO); close(fd);
        }
        if (atomic->output_file) {
            int flags = O_WRONLY | O_CREAT | (atomic->append_output ? O_APPEND : O_TRUNC);
            int fd = open(atomic->output_file, flags, 0644);
            if (fd < 0) { perror("open"); return; }
            dup2(fd, STDOUT_FILENO); close(fd);
        }
        handle_intrinsic(atomic->args);
        dup2(stdin_backup, STDIN_FILENO); dup2(stdout_backup, STDOUT_FILENO);
        close(stdin_backup); close(stdout_backup);
        return;
    }

    pid_t pid = fork();
    if (pid == 0) { // Child
        setpgid(0, 0);
        if (atomic->input_file) {
            int fd = open(atomic->input_file, O_RDONLY);
            if (fd < 0) { exit(EXIT_FAILURE); }
            dup2(fd, STDIN_FILENO); close(fd);
        }
        if (atomic->output_file) {
            int flags = O_WRONLY | O_CREAT | (atomic->append_output ? O_APPEND : O_TRUNC);
            int fd = open(atomic->output_file, flags, 0644);
            if (fd < 0) { fprintf(stderr, "Unable to create file for writing\n"); exit(EXIT_FAILURE); }
            dup2(fd, STDOUT_FILENO); close(fd);
        }
        if (is_background) {
            int null_fd = open("/dev/null", O_RDONLY);
            if (null_fd >= 0) { dup2(null_fd, STDIN_FILENO); close(null_fd); }
        }
        execvp(atomic->name, atomic->args);
        fprintf(stderr, "%s: Command not found!\n", atomic->name);
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Parent
        if (is_background) {
            char cmd_str[MAX_CMD_LEN] = {0};
            for(int i = 0; atomic->args[i] != NULL; i++) { strcat(cmd_str, atomic->args[i]); strcat(cmd_str, " "); }
            add_job(pid, cmd_str, RUNNING);
            printf("[%d] %d\n", job_count, pid);
        } else {
            foreground_pgid = pid;
            waitpid(pid, NULL, WUNTRACED);
            foreground_pgid = 0;
        }
    }
}


// #include "shell.h"

// // Forward declaration for helper function
// void execute_cmd_group(CmdGroup* group, int is_background);

// void execute_atomic_cmd(AtomicCmd* atomic, int is_background) {
//     if (atomic->name == NULL) return;
    
//     if (atomic->input_file) {
//         if (access(atomic->input_file, R_OK) == -1) {
//             fprintf(stderr, "No such file or directory!\n");
//             return;
//         }
//     }

//     if (is_intrinsic(atomic->name)) {
//         int stdin_backup = dup(STDIN_FILENO), stdout_backup = dup(STDOUT_FILENO);
//         if (atomic->input_file) {
//             int fd = open(atomic->input_file, O_RDONLY);
//             if (fd < 0) { perror(atomic->input_file); return; }
//             dup2(fd, STDIN_FILENO); close(fd);
//         }
//         if (atomic->output_file) {
//             int flags = O_WRONLY | O_CREAT | (atomic->append_output ? O_APPEND : O_TRUNC);
//             int fd = open(atomic->output_file, flags, 0644);
//             if (fd < 0) { perror("open"); return; }
//             dup2(fd, STDOUT_FILENO); close(fd);
//         }
//         handle_intrinsic(atomic->args);
//         dup2(stdin_backup, STDIN_FILENO); dup2(stdout_backup, STDOUT_FILENO);
//         close(stdin_backup); close(stdout_backup);
//         return;
//     }

//     pid_t pid = fork();
//     if (pid == 0) { // Child
//         setpgid(0, 0);
//         if (atomic->input_file) {
//             int fd = open(atomic->input_file, O_RDONLY);
//             if (fd < 0) { exit(1); }
//             dup2(fd, STDIN_FILENO); close(fd);
//         }
//         if (atomic->output_file) {
//             int flags = O_WRONLY | O_CREAT | (atomic->append_output ? O_APPEND : O_TRUNC);
//             int fd = open(atomic->output_file, flags, 0644);
//             if (fd < 0) { fprintf(stderr, "Unable to create file for writing\n"); exit(1); }
//             dup2(fd, STDOUT_FILENO); close(fd);
//         }
//         if (is_background) {
//             int null_fd = open("/dev/null", O_RDONLY);
//             if (null_fd >= 0) { dup2(null_fd, STDIN_FILENO); close(null_fd); }
//         }
//         execvp(atomic->name, atomic->args);
//         fprintf(stderr, "%s: Command not found!\n", atomic->name);
//         exit(1);
//     } else if (pid > 0) { // Parent
//         if (is_background) {
//             char cmd_str[MAX_CMD_LEN] = {0};
//             for(int i = 0; atomic->args[i] != NULL; i++) { strcat(cmd_str, atomic->args[i]); strcat(cmd_str, " "); }
//             add_job(pid, cmd_str, RUNNING);
//             printf("[%d] %d\n", job_count, pid);
//         } else {
//             foreground_pgid = pid;
//             waitpid(pid, NULL, WUNTRACED);
//             foreground_pgid = 0;
//         }
//     }
// }

// void execute_cmd_group(CmdGroup* group, int is_background) {
//     if (group->atomic_count == 0) return;
//     if (group->atomic_count == 1) {
//         execute_atomic_cmd(&group->atomics[0], is_background);
//         return;
//     }

//     int num_pipes = group->atomic_count - 1;
//     int pipe_fds[2];
//     int in_fd = STDIN_FILENO;
//     pid_t pids[group->atomic_count];

//     for (int i = 0; i < group->atomic_count; i++) {
//         if (i < num_pipes) pipe(pipe_fds);
        
//         pid_t pid = fork();
//         pids[i] = pid;
//         if (pid == 0) { // Child
//             setpgid(0, getpid());
//             if (i > 0) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
//             if (i < num_pipes) { dup2(pipe_fds[1], STDOUT_FILENO); close(pipe_fds[0]); close(pipe_fds[1]); }

//             AtomicCmd* atomic = &group->atomics[i];
//             if (i == 0 && atomic->input_file) {
//                  int fd = open(atomic->input_file, O_RDONLY);
//                  if(fd < 0) { perror(atomic->input_file); exit(1); }
//                  dup2(fd, STDIN_FILENO); close(fd);
//             }
//             if (i == num_pipes && atomic->output_file) {
//                  int flags = O_WRONLY | O_CREAT | (atomic->append_output ? O_APPEND : O_TRUNC);
//                  int fd = open(atomic->output_file, flags, 0644);
//                  if(fd < 0) { perror(atomic->output_file); exit(1); }
//                  dup2(fd, STDOUT_FILENO); close(fd);
//             }
            
//             if (is_intrinsic(atomic->name)) {
//                 handle_intrinsic(atomic->args);
//                 exit(EXIT_SUCCESS);
//             } else {
//                 execvp(atomic->name, atomic->args);
//                 fprintf(stderr, "%s: Command not found!\n", atomic->name);
//                 exit(EXIT_FAILURE);
//             }
//         }
//         if (i > 0) close(in_fd);
//         if (i < num_pipes) { in_fd = pipe_fds[0]; close(pipe_fds[1]); }
//     }

//     if (!is_background) {
//         foreground_pgid = pids[0];
//         for (int i = 0; i < group->atomic_count; i++) waitpid(pids[i], NULL, WUNTRACED);
//         foreground_pgid = 0;
//     } else {
//         char cmd_str[MAX_CMD_LEN] = {0};
//         strcpy(cmd_str, group->atomics[0].name); strcat(cmd_str, " | ...");
//         add_job(pids[0], cmd_str, RUNNING);
//         printf("[%d] %d\n", job_count, pids[0]);
//     }
// }

// void execute_shell_cmd(ShellCmd* cmd) {
//     if (cmd->block_count == 0) return;
//     for (int i = 0; i < cmd->block_count; i++) {
//         CommandBlock* block = &cmd->blocks[i];
//         if (block->group.atomic_count > 0) {
//             execute_cmd_group(&block->group, block->is_background);
//         }
//     }
// }

// // // #include "shell.h"

// // // void execute_atomic_cmd(AtomicCmd* atomic, int is_background) {
// // //     if (atomic->name == NULL) return;
    
// // //     if (atomic->input_file) {
// // //         if (access(atomic->input_file, R_OK) == -1) {
// // //             fprintf(stderr, "No such file or directory!\n");
// // //             return;
// // //         }
// // //     }

// // //     if (is_intrinsic(atomic->name)) {
// // //         int stdin_backup = dup(STDIN_FILENO), stdout_backup = dup(STDOUT_FILENO);
// // //         if (atomic->input_file) {
// // //             int fd = open(atomic->input_file, O_RDONLY);
// // //             if (fd < 0) { perror(atomic->input_file); return; }
// // //             dup2(fd, STDIN_FILENO); close(fd);
// // //         }
// // //         if (atomic->output_file) {
// // //             int flags = O_WRONLY | O_CREAT | (atomic->append_output ? O_APPEND : O_TRUNC);
// // //             int fd = open(atomic->output_file, flags, 0644);
// // //             if (fd < 0) { perror("open"); return; }
// // //             dup2(fd, STDOUT_FILENO); close(fd);
// // //         }
// // //         handle_intrinsic(atomic->args);
// // //         dup2(stdin_backup, STDIN_FILENO); dup2(stdout_backup, STDOUT_FILENO);
// // //         close(stdin_backup); close(stdout_backup);
// // //         return;
// // //     }

// // //     pid_t pid = fork();
// // //     if (pid == 0) { // Child
// // //         setpgid(0, 0);
// // //         if (atomic->input_file) {
// // //             int fd = open(atomic->input_file, O_RDONLY);
// // //             if (fd < 0) { exit(1); }
// // //             dup2(fd, STDIN_FILENO); close(fd);
// // //         }
// // //         if (atomic->output_file) {
// // //             int flags = O_WRONLY | O_CREAT | (atomic->append_output ? O_APPEND : O_TRUNC);
// // //             int fd = open(atomic->output_file, flags, 0644);
// // //             if (fd < 0) { fprintf(stderr, "Unable to create file for writing\n"); exit(1); }
// // //             dup2(fd, STDOUT_FILENO); close(fd);
// // //         }
// // //         if (is_background) {
// // //             int null_fd = open("/dev/null", O_RDONLY);
// // //             if (null_fd >= 0) { dup2(null_fd, STDIN_FILENO); close(null_fd); }
// // //         }
// // //         execvp(atomic->name, atomic->args);
// // //         fprintf(stderr, "%s: Command not found!\n", atomic->name);
// // //         exit(1);
// // //     } else if (pid > 0) { // Parent
// // //         if (is_background) {
// // //             char cmd_str[MAX_CMD_LEN] = {0};
// // //             for(int i = 0; atomic->args[i] != NULL; i++) { strcat(cmd_str, atomic->args[i]); strcat(cmd_str, " "); }
// // //             add_job(pid, cmd_str, RUNNING);
// // //             printf("[%d] %d\n", job_count, pid);
// // //         } else {
// // //             foreground_pgid = pid;
// // //             waitpid(pid, NULL, WUNTRACED);
// // //             foreground_pgid = 0;
// // //         }
// // //     }
// // // }

// // // void execute_cmd_group(CmdGroup* group, int is_background) {
// // //     if (group->atomic_count == 0) return;
// // //     if (group->atomic_count == 1) {
// // //         execute_atomic_cmd(&group->atomics[0], is_background);
// // //         return;
// // //     }

// // //     int num_pipes = group->atomic_count - 1;
// // //     int pipe_fds[2];
// // //     int in_fd = STDIN_FILENO;
// // //     pid_t pids[group->atomic_count];

// // //     for (int i = 0; i < group->atomic_count; i++) {
// // //         if (i < num_pipes) pipe(pipe_fds);
        
// // //         pid_t pid = fork();
// // //         pids[i] = pid;
// // //         if (pid == 0) { // Child
// // //             setpgid(0, getpid());
// // //             if (i > 0) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
// // //             if (i < num_pipes) { dup2(pipe_fds[1], STDOUT_FILENO); close(pipe_fds[0]); close(pipe_fds[1]); }

// // //             AtomicCmd* atomic = &group->atomics[i];
// // //             if (i == 0 && atomic->input_file) {
// // //                  int fd = open(atomic->input_file, O_RDONLY);
// // //                  if(fd < 0) { perror(atomic->input_file); exit(1); }
// // //                  dup2(fd, STDIN_FILENO); close(fd);
// // //             }
// // //             if (i == num_pipes && atomic->output_file) {
// // //                  int flags = O_WRONLY | O_CREAT | (atomic->append_output ? O_APPEND : O_TRUNC);
// // //                  int fd = open(atomic->output_file, flags, 0644);
// // //                  if(fd < 0) { perror(atomic->output_file); exit(1); }
// // //                  dup2(fd, STDOUT_FILENO); close(fd);
// // //             }
            
// // //             if (is_intrinsic(atomic->name)) {
// // //                 handle_intrinsic(atomic->args);
// // //                 exit(EXIT_SUCCESS);
// // //             } else {
// // //                 execvp(atomic->name, atomic->args);
// // //                 fprintf(stderr, "%s: Command not found!\n", atomic->name);
// // //                 exit(EXIT_FAILURE);
// // //             }
// // //         }
// // //         if (i > 0) close(in_fd);
// // //         if (i < num_pipes) { in_fd = pipe_fds[0]; close(pipe_fds[1]); }
// // //     }

// // //     if (!is_background) {
// // //         foreground_pgid = pids[0];
// // //         for (int i = 0; i < group->atomic_count; i++) waitpid(pids[i], NULL, WUNTRACED);
// // //         foreground_pgid = 0;
// // //     } else {
// // //         char cmd_str[MAX_CMD_LEN] = {0};
// // //         strcpy(cmd_str, group->atomics[0].name); strcat(cmd_str, " | ...");
// // //         add_job(pids[0], cmd_str, RUNNING);
// // //         printf("[%d] %d\n", job_count, pids[0]);
// // //     }
// // // }

// // // void execute_shell_cmd(ShellCmd* cmd) {
// // //     if (cmd->group_count == 0) return;
// // //     for (int i = 0; i < cmd->group_count; i++) {
// // //         int is_bg = (i == cmd->group_count - 1) && cmd->background;
// // //         execute_cmd_group(&cmd->groups[i], is_bg);
// // //     }
// // // }
// // #include "shell.h"

// // void execute_atomic_cmd(AtomicCmd* atomic, int is_background) {
// //     if (atomic->name == NULL) return;
    
// //     if (atomic->input_file) {
// //         if (access(atomic->input_file, R_OK) == -1) {
// //             fprintf(stderr, "No such file or directory!\n");
// //             return;
// //         }
// //     }

// //     if (is_intrinsic(atomic->name)) {
// //         int stdin_backup = dup(STDIN_FILENO), stdout_backup = dup(STDOUT_FILENO);
// //         if (atomic->input_file) {
// //             int fd = open(atomic->input_file, O_RDONLY);
// //             if (fd < 0) { perror(atomic->input_file); return; }
// //             dup2(fd, STDIN_FILENO); close(fd);
// //         }
// //         if (atomic->output_file) {
// //             int flags = O_WRONLY | O_CREAT | (atomic->append_output ? O_APPEND : O_TRUNC);
// //             int fd = open(atomic->output_file, flags, 0644);
// //             if (fd < 0) { perror("open"); return; }
// //             dup2(fd, STDOUT_FILENO); close(fd);
// //         }
// //         handle_intrinsic(atomic->args);
// //         dup2(stdin_backup, STDIN_FILENO); dup2(stdout_backup, STDOUT_FILENO);
// //         close(stdin_backup); close(stdout_backup);
// //         return;
// //     }

// //     pid_t pid = fork();
// //     if (pid == 0) { // Child
// //         setpgid(0, 0);
// //         if (atomic->input_file) {
// //             int fd = open(atomic->input_file, O_RDONLY);
// //             if (fd < 0) { exit(1); }
// //             dup2(fd, STDIN_FILENO); close(fd);
// //         }
// //         if (atomic->output_file) {
// //             int flags = O_WRONLY | O_CREAT | (atomic->append_output ? O_APPEND : O_TRUNC);
// //             int fd = open(atomic->output_file, flags, 0644);
// //             if (fd < 0) { fprintf(stderr, "Unable to create file for writing\n"); exit(1); }
// //             dup2(fd, STDOUT_FILENO); close(fd);
// //         }
// //         if (is_background) {
// //             int null_fd = open("/dev/null", O_RDONLY);
// //             if (null_fd >= 0) { dup2(null_fd, STDIN_FILENO); close(null_fd); }
// //         }
// //         execvp(atomic->name, atomic->args);
// //         fprintf(stderr, "%s: Command not found!\n", atomic->name);
// //         exit(1);
// //     } else if (pid > 0) { // Parent
// //         if (is_background) {
// //             char cmd_str[MAX_CMD_LEN] = {0};
// //             for(int i = 0; atomic->args[i] != NULL; i++) { strcat(cmd_str, atomic->args[i]); strcat(cmd_str, " "); }
// //             add_job(pid, cmd_str, RUNNING);
// //             printf("[%d] %d\n", job_count, pid);
// //         } else {
// //             foreground_pgid = pid;
// //             waitpid(pid, NULL, WUNTRACED);
// //             foreground_pgid = 0;
// //         }
// //     }
// // }

// // void execute_cmd_group(CmdGroup* group, int is_background) {
// //     if (group->atomic_count == 0) return;
// //     if (group->atomic_count == 1) {
// //         execute_atomic_cmd(&group->atomics[0], is_background);
// //         return;
// //     }

// //     int num_pipes = group->atomic_count - 1;
// //     int pipe_fds[2];
// //     int in_fd = STDIN_FILENO;
// //     pid_t pids[group->atomic_count];

// //     for (int i = 0; i < group->atomic_count; i++) {
// //         if (i < num_pipes) pipe(pipe_fds);
        
// //         pid_t pid = fork();
// //         pids[i] = pid;
// //         if (pid == 0) { // Child
// //             setpgid(0, getpid());
// //             if (i > 0) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
// //             if (i < num_pipes) { dup2(pipe_fds[1], STDOUT_FILENO); close(pipe_fds[0]); close(pipe_fds[1]); }

// //             AtomicCmd* atomic = &group->atomics[i];
// //             if (i == 0 && atomic->input_file) {
// //                  int fd = open(atomic->input_file, O_RDONLY);
// //                  if(fd < 0) { perror(atomic->input_file); exit(1); }
// //                  dup2(fd, STDIN_FILENO); close(fd);
// //             }
// //             if (i == num_pipes && atomic->output_file) {
// //                  int flags = O_WRONLY | O_CREAT | (atomic->append_output ? O_APPEND : O_TRUNC);
// //                  int fd = open(atomic->output_file, flags, 0644);
// //                  if(fd < 0) { perror(atomic->output_file); exit(1); }
// //                  dup2(fd, STDOUT_FILENO); close(fd);
// //             }
            
// //             if (is_intrinsic(atomic->name)) {
// //                 handle_intrinsic(atomic->args);
// //                 exit(EXIT_SUCCESS);
// //             } else {
// //                 execvp(atomic->name, atomic->args);
// //                 fprintf(stderr, "%s: Command not found!\n", atomic->name);
// //                 exit(EXIT_FAILURE);
// //             }
// //         }
// //         if (i > 0) close(in_fd);
// //         if (i < num_pipes) { in_fd = pipe_fds[0]; close(pipe_fds[1]); }
// //     }

// //     if (!is_background) {
// //         foreground_pgid = pids[0];
// //         for (int i = 0; i < group->atomic_count; i++) waitpid(pids[i], NULL, WUNTRACED);
// //         foreground_pgid = 0;
// //     } else {
// //         char cmd_str[MAX_CMD_LEN] = {0};
// //         strcpy(cmd_str, group->atomics[0].name); strcat(cmd_str, " | ...");
// //         add_job(pids[0], cmd_str, RUNNING);
// //         printf("[%d] %d\n", job_count, pids[0]);
// //     }
// // }

// // void execute_shell_cmd(ShellCmd* cmd) {
// //     if (cmd->block_count == 0) return;
// //     for (int i = 0; i < cmd->block_count; i++) {
// //         CommandBlock* block = &cmd->blocks[i];
// //         execute_cmd_group(&block->group, block->is_background);
// //     }
// // }