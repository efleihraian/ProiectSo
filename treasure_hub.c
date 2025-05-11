#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>

#define MAX_CMD_LEN 256
#define CMD_FILE "comms.txt"

void run_score_calculator() {
    DIR *dir;
    struct dirent *entry;
    char path[256];

    dir = opendir("./hunts");
    if (!dir) {
        perror("opendir hunts");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            pid_t pid = fork();
            if (pid == 0) {
                snprintf(path, sizeof(path)+30, "./calculate_score.sh %s", entry->d_name);
                execl("/bin/sh", "sh", "-c", path, NULL);
                perror("execl calculate_score.sh");
                exit(1);
            }
        }
    }

    closedir(dir);

    // Wait for all children
    while (wait(NULL) > 0);
}

int main() {
    char input[MAX_CMD_LEN];
    pid_t monitor_pid = -1;
    FILE *cmd_fp;

    printf("Treasure Hunt Hub\n");
    printf("Type 'start_monitor' to begin.\n");

    while (1) {
        printf("> ");
        fgets(input, MAX_CMD_LEN, stdin);
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            monitor_pid = fork();
            if (monitor_pid == 0) {
                execl("./monitor", "monitor", NULL);
                perror("Failed to start monitor");
                exit(1);
            } else if (monitor_pid < 0) {
                perror("fork failed");
                exit(1);
            }

            // Clear command file
            cmd_fp = fopen(CMD_FILE, "w");
            if (cmd_fp == NULL) {
                perror("fopen");
                exit(1);
            }
            fclose(cmd_fp);

            printf("Monitor started. Available commands:\n");
            printf("- list_hunts\n");
            printf("- list_treasures <hunt_id>\n");
            printf("- view_treasure <hunt_id> <treasure_id>\n");
            printf("- calculate_score\n");
            printf("- stop_monitor\n");
            printf("- exit\n");
        } else if (strcmp(input, "calculate_score") == 0) {
            run_score_calculator();
        } else if (strcmp(input, "stop_monitor") == 0) {
            cmd_fp = fopen(CMD_FILE, "w");
            if (cmd_fp) {
                fprintf(cmd_fp, "stop_monitor\n");
                fclose(cmd_fp);
            }

            if (monitor_pid > 0) {
                waitpid(monitor_pid, NULL, 0);
                printf("Monitor stopped.\n");
		monitor_pid=-1;
    }
        } else if (strcmp(input, "exit") == 0) {
            if (monitor_pid > 0) {
                printf("Please stop the monitor first using 'stop_monitor'.\n");
            } else {
                printf("Exiting Treasure Hunt Hub.\n");
                break;
            }
        } else if (monitor_pid > 0) {
            cmd_fp = fopen(CMD_FILE, "w");
            if (!cmd_fp) {
                perror("fopen");
                continue;
            }
            fprintf(cmd_fp, "%s\n", input);
            fclose(cmd_fp);
            sleep(10); // slight delay for monitor to read
        } else {
            printf("Please start the monitor first using 'start_monitor'.\n");
        }
    }

    return 0;
}
