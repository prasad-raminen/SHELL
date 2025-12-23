#include "shell.h"

static char* history[MAX_HISTORY_SIZE];
static int history_count = 0;
static int history_start = 0;

void load_history() {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", getenv("HOME"), HISTORY_FILE);
    FILE* fp = fopen(path, "r");
    if (!fp) return;
    char* line = NULL;
    size_t len = 0;
    while(getline(&line, &len, fp) != -1) {
        line[strcspn(line, "\n")] = 0;
        add_to_history(line);
    }
    free(line);
    fclose(fp);
}

void save_history() {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", getenv("HOME"), HISTORY_FILE);
    FILE* fp = fopen(path, "w");
    if (!fp) return;
    for (int i = 0; i < history_count; i++) {
        fprintf(fp, "%s\n", history[(history_start + i) % MAX_HISTORY_SIZE]);
    }
    fclose(fp);
}

void add_to_history(const char* cmd) {
    if (history_count > 0 && strcmp(history[(history_start + history_count - 1) % MAX_HISTORY_SIZE], cmd) == 0) return;

    if (history_count < MAX_HISTORY_SIZE) {
        history[(history_start + history_count) % MAX_HISTORY_SIZE] = strdup(cmd);
        history_count++;
    } else {
        free(history[history_start]);
        history[history_start] = strdup(cmd);
        history_start = (history_start + 1) % MAX_HISTORY_SIZE;
    }
}

void print_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%s\n", history[(history_start + i) % MAX_HISTORY_SIZE]);
    }
}

void purge_history() {
    for (int i = 0; i < history_count; i++) {
        free(history[(history_start + i) % MAX_HISTORY_SIZE]);
        history[(history_start + i) % MAX_HISTORY_SIZE] = NULL;
    }
    history_count = 0;
    history_start = 0;
}

char* get_history_by_index(int index) {
    if (index < 1 || index > history_count) return NULL;
    int real_index = (history_start + history_count - index) % MAX_HISTORY_SIZE;
    return strdup(history[real_index]);
}

// #include "shell.h"

// // These variables are static to this file, encapsulating the history logic.
// static char* history[MAX_HISTORY_SIZE];
// static int history_count = 0;
// static int history_start = 0;

// void load_history() {
//     char path[PATH_MAX];
//     snprintf(path, sizeof(path), "%s/%s", getenv("HOME"), HISTORY_FILE);
//     FILE* fp = fopen(path, "r");
//     if (!fp) return;
//     char* line = NULL;
//     size_t len = 0;
//     while(getline(&line, &len, fp) != -1) {
//         line[strcspn(line, "\n")] = 0;
//         add_to_history(line);
//     }
//     free(line);
//     fclose(fp);
// }

// void save_history() {
//     char path[PATH_MAX];
//     snprintf(path, sizeof(path), "%s/%s", getenv("HOME"), HISTORY_FILE);
//     FILE* fp = fopen(path, "w");
//     if (!fp) return;
//     for (int i = 0; i < history_count; i++) {
//         fprintf(fp, "%s\n", history[(history_start + i) % MAX_HISTORY_SIZE]);
//     }
//     fclose(fp);
// }

// void add_to_history(const char* cmd) {
//     if (history_count > 0 && strcmp(history[(history_start + history_count - 1) % MAX_HISTORY_SIZE], cmd) == 0) return;

//     if (history_count < MAX_HISTORY_SIZE) {
//         history[(history_start + history_count) % MAX_HISTORY_SIZE] = strdup(cmd);
//         history_count++;
//     } else {
//         free(history[history_start]);
//         history[history_start] = strdup(cmd);
//         history_start = (history_start + 1) % MAX_HISTORY_SIZE;
//     }
// }

// void print_history() {
//     for (int i = 0; i < history_count; i++) {
//         printf("%s\n", history[(history_start + i) % MAX_HISTORY_SIZE]);
//     }
// }

// void purge_history() {
//     for (int i = 0; i < history_count; i++) {
//         free(history[(history_start + i) % MAX_HISTORY_SIZE]);
//         history[(history_start + i) % MAX_HISTORY_SIZE] = NULL;
//     }
//     history_count = 0;
//     history_start = 0;
// }

// char* get_history_by_index(int index) {
//     if (index < 1 || index > history_count) return NULL;
//     int real_index = (history_start + history_count - index) % MAX_HISTORY_SIZE;
//     return strdup(history[real_index]);
// }
