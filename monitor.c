#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define TMP_FILE "comms.txt"
#define MAX_LINE 256
#define MAX_ARGS 5

void run_treasure_manager(char *cmd_line) {
    char *args[6]; // max 5 args + NULL
    int arg_count = 0;

    args[arg_count++] = "./treasure_manager";

    char cmd_copy[MAX_LINE];
    strncpy(cmd_copy, cmd_line, MAX_LINE);
    cmd_copy[MAX_LINE - 1] = '\0';

    char *token = strtok(cmd_copy, " ");

    if (strcmp(token, "list_hunts") == 0) {
        args[arg_count++] = "--list_hunts";
    } else if (strcmp(token, "list_treasures") == 0) {
        char *hunt_id = strtok(NULL, " ");
        if (hunt_id) {
            args[arg_count++] = "--list";
            args[arg_count++] = hunt_id;
        } else {
            fprintf(stderr, "Err: missing huntID for list_treasures\n");
            return;
        }
    } else if (strcmp(token, "view_treasure") == 0) {
        char *hunt_id = strtok(NULL, " ");
        char *treasure_id = strtok(NULL, " ");
        if (hunt_id && treasure_id) {
            args[arg_count++] = "--view";
            args[arg_count++] = hunt_id;
            args[arg_count++] = treasure_id;
        } else {
            fprintf(stderr, "Err: missing args for view_treasure\n");
            return;
        }
    } else {
        fprintf(stderr, "Unknown command: %s\n", token);
        return;
    }

    args[arg_count] = NULL;

    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork failed");
    }
}

int main() {
    int monitor_active = 1;
    while (monitor_active) {
        FILE *fp = fopen(TMP_FILE, "r");
        if (!fp) {
            sleep(1);
            continue;
        }
        char line[MAX_LINE];
        if (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;

            if (strcmp(line, "stop_monitor") == 0) {
                monitor_active = 0;
                printf("Stopping monitor...\n");
                sleep(5);
                fclose(fp);
                fp = fopen(TMP_FILE, "w");
                if (fp) fclose(fp);
                break;
            }		
            run_treasure_manager(line);

            fclose(fp);
            fp = fopen(TMP_FILE, "w");
            if (fp) fclose(fp);
        } else {
            fclose(fp);
        }

        sleep(1);
    }

    return 0;
}

