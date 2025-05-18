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
    ///char path[256];

    dir = opendir("./hunts");
    if (!dir) {
        perror("opendir hunts");
        return;
    }

    printf("\n");  

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {

            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                continue;
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                continue;
            }

            if (pid == 0) {          
                close(pipefd[1]); 
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);

                execl("./calculate_score", "calculate_score", NULL);
                perror("execl");
                exit(1);
            } else {
                close(pipefd[0]); 
                write(pipefd[1], entry->d_name, strlen(entry->d_name));
                write(pipefd[1], "\n", 1); 
                close(pipefd[1]); 

                waitpid(pid, NULL, 0);  
            }
        }
    }

    closedir(dir);
}


int main() {
    char input[MAX_CMD_LEN];
    pid_t monitor_pid = -1;
    FILE *cmd_fp;

    printf("Treasure Hunt Hub\n");
    printf("Type 'start_monitor' to begin.\n");
    printf("Or type 'calculate_score'.\n");
    printf("Or type 'exit'.\n");

    while (1) {
        printf("> ");
        fgets(input, MAX_CMD_LEN, stdin);
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_pid != -1) {
                printf("Monitor is already running.\n");
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid == 0) {
                execl("./monitor", "monitor", NULL);
                perror("Failed to start monitor");
                exit(1);
            } else if (monitor_pid < 0) {
                perror("fork failed");
                exit(1);
            }

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
        }  else if (strcmp(input, "calculate_score") == 0) {
    		run_score_calculator();
    		if (monitor_pid >= 0) {
        		//printf(">pu ");
        		fflush(stdout);
    		}
        } else if (strcmp(input, "stop_monitor") == 0) {
            cmd_fp = fopen(CMD_FILE, "w");
            if (cmd_fp) {
                fprintf(cmd_fp, "stop_monitor\n");
                fclose(cmd_fp);
            }

            if (monitor_pid > 0) {
                waitpid(monitor_pid, NULL, 0);
                printf("Monitor stopped.\n");
                monitor_pid = -1;

                printf("Type 'start_monitor' to begin again.\n");
                printf("Or type 'calculate_score'.\n");
                printf("Or type 'exit'.\n");
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
            sleep(1); 
        } else {
            printf("Unknown command. Type 'start_monitor', 'calculate_score', or 'exit'.\n");
        }
    }

    return 0;
}

