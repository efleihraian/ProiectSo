#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

#define USERNAME_MAX 32
#define CLUE_MAX 256
#define MAX_PATH 512

typedef struct {
    int treasure_id;
    char username[USERNAME_MAX];
    float latitude;
    float longitude;
    char clue[CLUE_MAX];
    int value;
} Treasure;

// Function declarations
void list_hunts();
void add_treasure(const char *hunt_id);
void list_treasures(const char *hunt_id);
void view_treasure(const char *hunt_id, int treasure_id);
void remove_treasure(const char *hunt_id, int treasure_id);
void remove_hunt(const char *hunt_id);
void log_operation(const char *hunt_id, const char *message);
void create_symlink(const char *hunt_id);

int main(int argc, char *argv[]) {
    // Ensure "hunts" directory exists
    struct stat st = {0};
    if (stat("hunts", &st) == -1) {
        if (mkdir("hunts", 0755) == -1) {
            perror("mkdir hunts");
            exit(EXIT_FAILURE);
        }
    }

    if (argc < 2) {
        fprintf(stderr, "Usage:\n"
                        "--list_hunts\n"
                        "--add <hunt_id>\n"
                        "--list <hunt_id>\n"
                        "--view <hunt_id> <treasure_id>\n"
                        "--remove_treasure <hunt_id> <treasure_id>\n"
                        "--remove_hunt <hunt_id>\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "--list_hunts") == 0) {
        list_hunts();
    } else if (strcmp(argv[1], "--add") == 0 && argc >= 3) {
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0 && argc >= 3) {
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "--view") == 0 && argc >= 4) {
        view_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "--remove_treasure") == 0 && argc >= 4) {
        remove_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "--remove_hunt") == 0 && argc >= 3) {
        remove_hunt(argv[2]);
    } else {
        fprintf(stderr, "Unknown or incomplete command\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

void list_hunts() {
    DIR *dir = opendir("hunts");
    if (!dir) {
        perror("opendir hunts");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char filepath[MAX_PATH];
            snprintf(filepath, sizeof(filepath), "hunts/%s/treasures.dat", entry->d_name);
            int count = 0;
            int fd = open(filepath, O_RDONLY);
            if (fd != -1) {
                Treasure t;
                while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) count++;
                close(fd);
            }
            printf("%s (%d treasures)\n", entry->d_name, count);
        }
    }
    closedir(dir);
}

void log_operation(const char *hunt_id, const char *message) {
    char log_path[MAX_PATH];
    snprintf(log_path, sizeof(log_path), "hunts/%s/logged_hunt", hunt_id);

    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("open (logged_hunt)");
        return;
    }

    write(fd, message, strlen(message));
    close(fd);
}

void create_symlink(const char *hunt_id) {
    char target[MAX_PATH];
    char link_name[MAX_PATH];

    snprintf(target, sizeof(target), "hunts/%s/logged_hunt", hunt_id);
    snprintf(link_name, sizeof(link_name), "logged_hunt-%s", hunt_id);

    unlink(link_name);

    if (symlink(target, link_name) == -1) {
        perror("symlink");
    }
}

void add_treasure(const char* hunt_id) {
    char dir_path[MAX_PATH];
    char file_path[MAX_PATH];
    char log_path[MAX_PATH];
    Treasure t;

    snprintf(dir_path, sizeof(dir_path)+15, "hunts/%s", hunt_id);
    snprintf(file_path, sizeof(file_path)+15, "%s/treasures.dat", dir_path);
    snprintf(log_path, sizeof(log_path)+15, "%s/logged_hunt", dir_path);

    if (mkdir(dir_path, 0755) == -1 && errno != EEXIST) {
        perror("mkdir hunt");
        exit(EXIT_FAILURE);
    }

    printf("Enter Treasure ID: ");
    scanf("%d", &t.treasure_id);
    getchar();

    printf("Enter Username: ");
    fgets(t.username, USERNAME_MAX, stdin);
    t.username[strcspn(t.username, "\n")] = 0;

    printf("Enter Latitude: ");
    scanf("%f", &t.latitude);

    printf("Enter Longitude: ");
    scanf("%f", &t.longitude);
    getchar();

    printf("Enter Clue: ");
    fgets(t.clue, CLUE_MAX, stdin);
    t.clue[strcspn(t.clue, "\n")] = 0;

    printf("Enter Value: ");
    scanf("%d", &t.value);

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("open (treasures.dat)");
        exit(EXIT_FAILURE);
    }

    if (write(fd, &t, sizeof(Treasure)) != sizeof(Treasure)) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);

    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Added treasure ID %d by user %s\n", t.treasure_id, t.username);
    log_operation(hunt_id, log_msg);

    create_symlink(hunt_id);

    printf("Treasure added successfully.\n");
}

void list_treasures(const char *hunt_id) {
    char file_path[MAX_PATH];
    snprintf(file_path, sizeof(file_path), "hunts/%s/treasures.dat", hunt_id);

    struct stat st;
    if (stat(file_path, &st) == -1) {
        perror("stat");
        return;
    }

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("open (treasures.dat)");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("\nTreasure ID: %d\n", t.treasure_id);
    }
    printf("\n");
    close(fd);
}

void view_treasure(const char *hunt_id, int treasure_id) {
    char file_path[MAX_PATH];
    snprintf(file_path, sizeof(file_path), "hunts/%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("open (treasures.dat)");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.treasure_id == treasure_id) {
            printf("Treasure ID: %d\n", t.treasure_id);
            printf("Username: %s\n", t.username);
            printf("Coordinates: %.2f, %.2f\n", t.latitude, t.longitude);
            printf("Clue: %s\n", t.clue);
            printf("Value: %d\n\n", t.value);
            close(fd);
            return;
        }
    }

    printf("Treasure with ID %d not found.\n", treasure_id);
    close(fd);
}

void remove_treasure(const char *hunt_id, int treasure_id) {
    char file_path[MAX_PATH];
    snprintf(file_path, sizeof(file_path), "hunts/%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_RDWR);
    if (fd == -1) {
        perror("open (treasures.dat)");
        return;
    }

    char temp_path[MAX_PATH];
    snprintf(temp_path, sizeof(temp_path), "hunts/%s/temp_treasures.dat", hunt_id);
    int temp_fd = open(temp_path, O_WRONLY | O_CREAT, 0644);
    if (temp_fd == -1) {
        perror("open (temp file)");
        close(fd);
        return;
    }

    Treasure t;
    int treasure_found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.treasure_id == treasure_id) {
            treasure_found = 1;
            continue;
        }
        write(temp_fd, &t, sizeof(Treasure));
    }

    if (!treasure_found) {
        printf("Treasure with ID %d not found.\n", treasure_id);
    } else {
        printf("Treasure with ID %d removed successfully.\n", treasure_id);
        remove(file_path);
        rename(temp_path, file_path);
    }
    char msg[256];
    sprintf(msg, "Deleted treasure ID %d by user %s\n", t.treasure_id, t.username);
    log_operation(hunt_id, msg);
    close(fd);
    close(temp_fd);
}

void remove_hunt(const char *hunt_id) {
    char file_path[MAX_PATH];

    const char *files[] = {
        "treasures.dat",
        "logged_hunt",
        "temp_treasures.dat"
    };

    for (int i = 0; i < sizeof(files)/sizeof(files[0]); i++) {
        snprintf(file_path, sizeof(file_path), "hunts/%s/%s", hunt_id, files[i]);
        unlink(file_path);
    }

    char dir_path[MAX_PATH];
    snprintf(dir_path, sizeof(dir_path), "hunts/%s", hunt_id);
    if (rmdir(dir_path) == -1) {
        perror("rmdir");
        return;
    }

    char link_name[MAX_PATH];
    snprintf(link_name, sizeof(link_name), "logged_hunt-%s", hunt_id);
    if (unlink(link_name) == -1) {
        if (errno != ENOENT)
            perror("unlink (symlink)");
    }

    printf("Hunt '%s' removed.\n", hunt_id);
}

