#include "shell.h"

void parse_cmd_group(CmdGroup* group, char* str); // Forward declaration

void free_shell_cmd(ShellCmd* cmd) {
    if (!cmd) return;
    for (int i = 0; i < cmd->block_count; i++) {
        CmdGroup* group = &cmd->blocks[i].group;
        for (int j = 0; j < group->atomic_count; j++) {
            free(group->atomics[j].name);
            for (int k = 0; group->atomics[j].args[k] != NULL; k++) {
                free(group->atomics[j].args[k]);
            }
            free(group->atomics[j].args);
            free(group->atomics[j].input_file);
            free(group->atomics[j].output_file);
        }
        free(group->atomics);
    }
    free(cmd->blocks);
    free(cmd);
}

ShellCmd* parse_input(const char* input_str) {
    ShellCmd* shell_cmd = calloc(1, sizeof(ShellCmd));
    shell_cmd->blocks = malloc(sizeof(CommandBlock) * MAX_ARGS);
    shell_cmd->block_count = 0;

    char* mutable_input = strdup(input_str);
    char* current_pos = mutable_input;

    while (*current_pos != '\0') {
        // Find the next separator (& or ;)
        char* separator = strpbrk(current_pos, ";&");
        
        CommandBlock* block = &shell_cmd->blocks[shell_cmd->block_count++];
        block->is_background = 0;
        
        if (separator != NULL) {
            if (*separator == '&') {
                block->is_background = 1;
            }
            *separator = '\0'; // Terminate the substring for the current block
            
            parse_cmd_group(&block->group, current_pos);
            current_pos = separator + 1;
        } else {
            // This is the last command block on the line
            parse_cmd_group(&block->group, current_pos);
            break; // No more separators, exit loop
        }
    }

    free(mutable_input);
    return shell_cmd;
}

void parse_cmd_group(CmdGroup* group, char* str) {
    group->atomics = malloc(sizeof(AtomicCmd) * MAX_ARGS);
    group->atomic_count = 0;

    char* atomic_token;
    char* saveptr;
    for (atomic_token = strtok_r(str, "|", &saveptr); atomic_token; atomic_token = strtok_r(NULL, "|", &saveptr)) {
        AtomicCmd* current_atomic = &group->atomics[group->atomic_count++];
        memset(current_atomic, 0, sizeof(AtomicCmd));
        current_atomic->args = calloc(MAX_ARGS, sizeof(char*));

        char* arg_token;
        char* saveptr2;
        int arg_idx = 0;
        char* temp_atomic_str = strdup(atomic_token);

        for (arg_token = strtok_r(temp_atomic_str, " \t\r\n", &saveptr2); arg_token; arg_token = strtok_r(NULL, " \t\r\n", &saveptr2)) {
            if (current_atomic->redirect_error) continue;

            if (strcmp(arg_token, "<") == 0) {
                arg_token = strtok_r(NULL, " \t\r\n", &saveptr2);
                if (arg_token) {
                    if (access(arg_token, R_OK) != 0) {
                        current_atomic->redirect_error = 1;
                    }
                    free(current_atomic->input_file);
                    current_atomic->input_file = strdup(arg_token);
                }
            } else if (strcmp(arg_token, ">") == 0 || strcmp(arg_token, ">>") == 0) {
                int is_append = (strcmp(arg_token, ">>") == 0);
                arg_token = strtok_r(NULL, " \t\r\n", &saveptr2);
                if (arg_token) {
                    int fd = open(arg_token, O_WRONLY | O_CREAT, 0644);
                    if (fd < 0) {
                        current_atomic->redirect_error = 2;
                    } else {
                        close(fd);
                    }
                    free(current_atomic->output_file);
                    current_atomic->output_file = strdup(arg_token);
                    current_atomic->append_output = is_append;
                }
            } else {
                if (arg_idx == 0) current_atomic->name = strdup(arg_token);
                current_atomic->args[arg_idx++] = strdup(arg_token);
            }
        }
        current_atomic->args[arg_idx] = NULL;
        free(temp_atomic_str);
    }
}

// // // #include "shell.h"

// // // int is_special_char(const char* token) {
// // //     return strcmp(token, "|") == 0 || strcmp(token, "&") == 0 || strcmp(token, ";") == 0 ||
// // //            strcmp(token, "<") == 0 || strcmp(token, ">") == 0 || strcmp(token, ">>") == 0;
// // // }

// // // int is_valid_syntax(const char* input) {
// // //     char temp_input[MAX_CMD_LEN];
// // //     strcpy(temp_input, input);

// // //     char* token;
// // //     char* saveptr;
// // //     int last_was_op = 1;

// // //     token = strtok_r(temp_input, " \t\r\n", &saveptr);
// // //     if (token == NULL) return 1;

// // //     do {
// // //         if (is_special_char(token)) {
// // //             if (last_was_op && strcmp(token, "&") != 0) return 0;
// // //             last_was_op = 1;
// // //         } else {
// // //             last_was_op = 0;
// // //         }
// // //         token = strtok_r(NULL, " \t\r\n", &saveptr);
// // //     } while (token != NULL);

// // //     return 1;
// // // }

// // // void free_shell_cmd(ShellCmd* cmd) {
// // //     if (!cmd) return;
// // //     for (int i = 0; i < cmd->group_count; i++) {
// // //         for (int j = 0; j < cmd->groups[i].atomic_count; j++) {
// // //             free(cmd->groups[i].atomics[j].name);
// // //             for (int k = 0; cmd->groups[i].atomics[j].args[k] != NULL; k++) {
// // //                 free(cmd->groups[i].atomics[j].args[k]);
// // //             }
// // //             free(cmd->groups[i].atomics[j].args);
// // //             free(cmd->groups[i].atomics[j].input_file);
// // //             free(cmd->groups[i].atomics[j].output_file);
// // //         }
// // //         free(cmd->groups[i].atomics);
// // //     }
// // //     free(cmd->groups);
// // //     free(cmd);
// // // }

// // // ShellCmd* parse_input(const char* input_str) {
// // //     ShellCmd* shell_cmd = calloc(1, sizeof(ShellCmd));
// // //     shell_cmd->groups = malloc(sizeof(CmdGroup) * MAX_ARGS);
// // //     shell_cmd->group_count = 0;

// // //     char* mutable_input = strdup(input_str);
// // //     char* command_str = mutable_input;

// // //     if (strlen(command_str) > 0 && command_str[strlen(command_str) - 1] == '&') {
// // //         shell_cmd->background = 1;
// // //         command_str[strlen(command_str) - 1] = '\0';
// // //     }

// // //     char* group_token;
// // //     char* saveptr1;
// // //     for (group_token = strtok_r(command_str, ";", &saveptr1); group_token; group_token = strtok_r(NULL, ";", &saveptr1)) {
// // //         CmdGroup* current_group = &shell_cmd->groups[shell_cmd->group_count++];
// // //         current_group->atomics = malloc(sizeof(AtomicCmd) * MAX_ARGS);
// // //         current_group->atomic_count = 0;

// // //         char* atomic_token;
// // //         char* saveptr2;
// // //         char* group_token_dup = strdup(group_token);
// // //         for (atomic_token = strtok_r(group_token_dup, "|", &saveptr2); atomic_token; atomic_token = strtok_r(NULL, "|", &saveptr2)) {
// // //             AtomicCmd* current_atomic = &current_group->atomics[current_group->atomic_count++];
// // //             memset(current_atomic, 0, sizeof(AtomicCmd));
// // //             current_atomic->args = calloc(MAX_ARGS, sizeof(char*));
            
// // //             char* arg_token;
// // //             char* saveptr3;
// // //             int arg_idx = 0;
// // //             char* temp_atomic_str = strdup(atomic_token);

// // //             for (arg_token = strtok_r(temp_atomic_str, " \t\r\n", &saveptr3); arg_token; arg_token = strtok_r(NULL, " \t\r\n", &saveptr3)) {
// // //                 if (strcmp(arg_token, "<") == 0) {
// // //                     arg_token = strtok_r(NULL, " \t\r\n", &saveptr3);
// // //                     if (arg_token) current_atomic->input_file = strdup(arg_token);
// // //                 } else if (strcmp(arg_token, ">") == 0) {
// // //                     arg_token = strtok_r(NULL, " \t\r\n", &saveptr3);
// // //                     if (arg_token) current_atomic->output_file = strdup(arg_token);
// // //                     current_atomic->append_output = 0;
// // //                 } else if (strcmp(arg_token, ">>") == 0) {
// // //                     arg_token = strtok_r(NULL, " \t\r\n", &saveptr3);
// // //                     if (arg_token) current_atomic->output_file = strdup(arg_token);
// // //                     current_atomic->append_output = 1;
// // //                 } else {
// // //                     if (arg_idx == 0) current_atomic->name = strdup(arg_token);
// // //                     current_atomic->args[arg_idx++] = strdup(arg_token);
// // //                 }
// // //             }
// // //             free(temp_atomic_str);
// // //         }
// // //         free(group_token_dup);
// // //     }

// // //     free(mutable_input);
// // //     return shell_cmd;
// // // }
// // #include "shell.h"

// // int is_special_char(const char* token) {
// //     return strcmp(token, "|") == 0 || strcmp(token, "&") == 0 || strcmp(token, ";") == 0 ||
// //            strcmp(token, "<") == 0 || strcmp(token, ">") == 0 || strcmp(token, ">>") == 0;
// // }

// // int is_valid_syntax(const char* input) {
// //     char temp_input[MAX_CMD_LEN];
// //     strcpy(temp_input, input);
// //     // This is a simplified validator. The robust execution logic is the primary guard.
// //     return 1;
// // }

// // void free_shell_cmd(ShellCmd* cmd) {
// //     if (!cmd) return;
// //     for (int i = 0; i < cmd->block_count; i++) {
// //         CmdGroup* group = &cmd->blocks[i].group;
// //         for (int j = 0; j < group->atomic_count; j++) {
// //             free(group->atomics[j].name);
// //             for (int k = 0; group->atomics[j].args[k] != NULL; k++) {
// //                 free(group->atomics[j].args[k]);
// //             }
// //             free(group->atomics[j].args);
// //             free(group->atomics[j].input_file);
// //             free(group->atomics[j].output_file);
// //         }
// //         free(group->atomics);
// //     }
// //     free(cmd->blocks);
// //     free(cmd);
// // }

// // // Parses a single command group (pipeline)
// // void parse_cmd_group(CmdGroup* group, char* str) {
// //     group->atomics = malloc(sizeof(AtomicCmd) * MAX_ARGS);
// //     group->atomic_count = 0;

// //     char* atomic_token;
// //     char* saveptr;
// //     for (atomic_token = strtok_r(str, "|", &saveptr); atomic_token; atomic_token = strtok_r(NULL, "|", &saveptr)) {
// //         AtomicCmd* current_atomic = &group->atomics[group->atomic_count++];
// //         memset(current_atomic, 0, sizeof(AtomicCmd));
// //         current_atomic->args = calloc(MAX_ARGS, sizeof(char*));

// //         char* arg_token;
// //         char* saveptr2;
// //         int arg_idx = 0;
// //         char* temp_atomic_str = strdup(atomic_token);

// //         for (arg_token = strtok_r(temp_atomic_str, " \t\r\n", &saveptr2); arg_token; arg_token = strtok_r(NULL, " \t\r\n", &saveptr2)) {
// //             if (strcmp(arg_token, "<") == 0) {
// //                 arg_token = strtok_r(NULL, " \t\r\n", &saveptr2);
// //                 if (arg_token) current_atomic->input_file = strdup(arg_token);
// //             } else if (strcmp(arg_token, ">") == 0) {
// //                 arg_token = strtok_r(NULL, " \t\r\n", &saveptr2);
// //                 if (arg_token) current_atomic->output_file = strdup(arg_token);
// //                 current_atomic->append_output = 0;
// //             } else if (strcmp(arg_token, ">>") == 0) {
// //                 arg_token = strtok_r(NULL, " \t\r\n", &saveptr2);
// //                 if (arg_token) current_atomic->output_file = strdup(arg_token);
// //                 current_atomic->append_output = 1;
// //             } else {
// //                 if (arg_idx == 0) current_atomic->name = strdup(arg_token);
// //                 current_atomic->args[arg_idx++] = strdup(arg_token);
// //             }
// //         }
// //         free(temp_atomic_str);
// //     }
// // }

// // ShellCmd* parse_input(const char* input_str) {
// //     ShellCmd* shell_cmd = calloc(1, sizeof(ShellCmd));
// //     shell_cmd->blocks = malloc(sizeof(CommandBlock) * MAX_ARGS);
// //     shell_cmd->block_count = 0;

// //     char* mutable_input = strdup(input_str);
// //     char* command_str = mutable_input;
// //     int len = strlen(command_str);

// //     // Trim trailing whitespace and final '&'
// //     while(len > 0 && isspace(command_str[len-1])) {
// //         command_str[--len] = '\0';
// //     }

// //     int final_bg = 0;
// //     if (len > 0 && command_str[len-1] == '&') {
// //         final_bg = 1;
// //         command_str[--len] = '\0';
// //     }
// //      while(len > 0 && isspace(command_str[len-1])) {
// //         command_str[--len] = '\0';
// //     }

// //     char* group_str;
// //     char* saveptr;
// //     for(group_str = strtok_r(command_str, ";", &saveptr); group_str; group_str = strtok_r(NULL, ";", &saveptr)) {
// //         char *bg_group_str;
// //         char *saveptr2;
// //         char *group_str_dup = strdup(group_str);

// //         for(bg_group_str = strtok_r(group_str_dup, "&", &saveptr2); bg_group_str; bg_group_str = strtok_r(NULL, "&", &saveptr2)) {
// //             CommandBlock* block = &shell_cmd->blocks[shell_cmd->block_count++];
            
// //             char* next_char = bg_group_str + strlen(bg_group_str);
// //             // Check if there are more tokens separated by '&'
// //             if (*(saveptr2 - 1) == '&') {
// //                  block->is_background = 1;
// //             } else {
// //                  block->is_background = 0;
// //             }

// //             parse_cmd_group(&block->group, bg_group_str);
// //         }
// //         free(group_str_dup);
// //     }
    
// //     // Apply final background operator if it exists
// //     if(final_bg && shell_cmd->block_count > 0) {
// //         shell_cmd->blocks[shell_cmd->block_count - 1].is_background = 1;
// //     }

// //     free(mutable_input);
// //     return shell_cmd;
// // }

// #include "shell.h"

// // Forward declaration for a helper function used only in this file.
// void parse_cmd_group(CmdGroup* group, char* str);

// void free_shell_cmd(ShellCmd* cmd) {
//     if (!cmd) return;
//     for (int i = 0; i < cmd->block_count; i++) {
//         CmdGroup* group = &cmd->blocks[i].group;
//         for (int j = 0; j < group->atomic_count; j++) {
//             free(group->atomics[j].name);
//             for (int k = 0; group->atomics[j].args[k] != NULL; k++) {
//                 free(group->atomics[j].args[k]);
//             }
//             free(group->atomics[j].args);
//             free(group->atomics[j].input_file);
//             free(group->atomics[j].output_file);
//         }
//         free(group->atomics);
//     }
//     free(cmd->blocks);
//     free(cmd);
// }

// ShellCmd* parse_input(const char* input_str) {
//     ShellCmd* shell_cmd = calloc(1, sizeof(ShellCmd));
//     shell_cmd->blocks = malloc(sizeof(CommandBlock) * MAX_ARGS);
//     shell_cmd->block_count = 0;

//     char* mutable_input = strdup(input_str);
//     char* current_pos = mutable_input;

//     while (*current_pos != '\0') {
//         // Find the next separator (& or ;)
//         char* separator = strpbrk(current_pos, ";&");
        
//         CommandBlock* block = &shell_cmd->blocks[shell_cmd->block_count++];
//         block->is_background = 0;
        
//         if (separator != NULL) {
//             if (*separator == '&') {
//                 block->is_background = 1;
//             }
//             *separator = '\0'; // Terminate the substring for the current block
            
//             parse_cmd_group(&block->group, current_pos);
//             current_pos = separator + 1;
//         } else {
//             // This is the last command block on the line
//             parse_cmd_group(&block->group, current_pos);
//             break; // No more separators, exit loop
//         }
//     }

//     free(mutable_input);
//     return shell_cmd;
// }

// // Helper to parse a single command group (a pipeline)
// void parse_cmd_group(CmdGroup* group, char* str) {
//     group->atomics = malloc(sizeof(AtomicCmd) * MAX_ARGS);
//     group->atomic_count = 0;

//     char* atomic_token;
//     char* saveptr;
//     for (atomic_token = strtok_r(str, "|", &saveptr); atomic_token; atomic_token = strtok_r(NULL, "|", &saveptr)) {
//         AtomicCmd* current_atomic = &group->atomics[group->atomic_count++];
//         memset(current_atomic, 0, sizeof(AtomicCmd));
//         current_atomic->args = calloc(MAX_ARGS, sizeof(char*));

//         char* arg_token;
//         char* saveptr2;
//         int arg_idx = 0;
//         char* temp_atomic_str = strdup(atomic_token);

//         for (arg_token = strtok_r(temp_atomic_str, " \t\r\n", &saveptr2); arg_token; arg_token = strtok_r(NULL, " \t\r\n", &saveptr2)) {
//             if (strcmp(arg_token, "<") == 0) {
//                 arg_token = strtok_r(NULL, " \t\r\n", &saveptr2);
//                 if (arg_token) current_atomic->input_file = strdup(arg_token);
//             } else if (strcmp(arg_token, ">") == 0) {
//                 arg_token = strtok_r(NULL, " \t\r\n", &saveptr2);
//                 if (arg_token) current_atomic->output_file = strdup(arg_token);
//                 current_atomic->append_output = 0;
//             } else if (strcmp(arg_token, ">>") == 0) {
//                 arg_token = strtok_r(NULL, " \t\r\n", &saveptr2);
//                 if (arg_token) current_atomic->output_file = strdup(arg_token);
//                 current_atomic->append_output = 1;
//             } else {
//                 if (arg_idx == 0) current_atomic->name = strdup(arg_token);
//                 current_atomic->args[arg_idx++] = strdup(arg_token);
//             }
//         }
//         free(temp_atomic_str);
//     }
// }