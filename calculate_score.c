#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_PATH 512
#define USERNAME_MAX 32
#define CLUE_MAX 256
#define MAX_USERS 100

typedef struct {
    int treasure_id;
    char username[USERNAME_MAX];
    float latitude;
    float longitude;
    char clue[CLUE_MAX];
    int value;
} Treasure;

typedef struct {
    char username[USERNAME_MAX];
    int score;
} UserScore;

int main() {
    char hunt_id[64];

    if (!fgets(hunt_id, sizeof(hunt_id), stdin)) {
        fprintf(stderr, "Err reading hunt_id\n");
        return 1;
    }
    hunt_id[strcspn(hunt_id, "\n")] = 0; 

    char file_path[MAX_PATH];
    snprintf(file_path, sizeof(file_path), "hunts/%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Could not open treasures.dat");
        return 1;
    }

    Treasure t;
    UserScore users[MAX_USERS];
    int user_count = 0;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        int found = 0;
        for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i].username, t.username) == 0) {
                users[i].score += t.value;
                found = 1;
                break;
            }
        }
        if (!found && user_count < MAX_USERS) {
            strcpy(users[user_count].username, t.username);
            users[user_count].score = t.value;
            user_count++;
        }
    }

    close(fd);

    printf("%s:\n", hunt_id);
    for (int i = 0; i < user_count; i++) {
        printf("%s total score %d\n", users[i].username, users[i].score);
    }

    return 0;
}

