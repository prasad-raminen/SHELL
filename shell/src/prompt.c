#include "shell.h"

void print_prompt() {
    char username[LOGIN_NAME_MAX];
    char hostname[HOST_NAME_MAX];
    char cwd[PATH_MAX];
    char display_path[PATH_MAX];

    getlogin_r(username, sizeof(username));
    gethostname(hostname, sizeof(hostname));
    getcwd(cwd, sizeof(cwd));

    if (strncmp(SHELL_HOME, cwd, strlen(SHELL_HOME)) == 0) {
        snprintf(display_path, sizeof(display_path), "~%s", cwd + strlen(SHELL_HOME));
    } else {
        strncpy(display_path, cwd, sizeof(display_path));
    }

    printf("<%s@%s:%s> ", username, hostname, display_path);
    fflush(stdout);
}

// #include "shell.h"

// void print_prompt() {
//     char username[LOGIN_NAME_MAX];
//     char hostname[HOST_NAME_MAX];
//     char cwd[PATH_MAX];
//     char display_path[PATH_MAX];

//     getlogin_r(username, sizeof(username));
//     gethostname(hostname, sizeof(hostname));
//     getcwd(cwd, sizeof(cwd));

//     if (strncmp(SHELL_HOME, cwd, strlen(SHELL_HOME)) == 0) {
//         snprintf(display_path, sizeof(display_path), "~%s", cwd + strlen(SHELL_HOME));
//     } else {
//         strncpy(display_path, cwd, sizeof(display_path));
//     }

//     printf("<%s@%s:%s> ", username, hostname, display_path);
//     fflush(stdout);
// }
