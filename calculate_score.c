#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define MAX_LINE 256
#define MAX_USERS 100

typedef struct {
    char username[50];
    int score;
} UserScore;

int find_user(UserScore users[], int count, const char* name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(users[i].username, name) == 0)
            return i;
    }
    return -1;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char hunt_path[256];
    snprintf(hunt_path, sizeof(hunt_path), "hunts/%s", argv[1]);

    DIR* dir = opendir(hunt_path);
    if (!dir) {
        perror("Failed to open hunt directory");
        return 1;
    }

    struct dirent* entry;
    UserScore users[MAX_USERS];
    int user_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG)
            continue;

        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/%s", hunt_path, entry->d_name);

        FILE* f = fopen(file_path, "r");
        if (!f) continue;

        char line[MAX_LINE];
        char owner[50] = "";
        int value = 0;

        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "owner=", 6) == 0) {
                sscanf(line, "owner=%49[^,\n]", owner);
            } else if (strncmp(line, "value=", 6) == 0) {
                sscanf(line, "value=%d", &value);
            }
        }

        fclose(f);

        if (strlen(owner) > 0 && value > 0) {
            int idx = find_user(users, user_count, owner);
            if (idx >= 0) {
                users[idx].score += value;
            } else {
                strncpy(users[user_count].username, owner, sizeof(users[user_count].username) - 1);
                users[user_count].score = value;
                user_count++;
            }
        }
    }

    closedir(dir);

    printf("Scores for hunt '%s':\n", argv[1]);
    for (int i = 0; i < user_count; i++) {
        printf("%s: %d\n", users[i].username, users[i].score);
    }

    return 0;
}
