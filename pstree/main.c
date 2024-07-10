#include <dirent.h>  // 包含用于处理目录流的函数和常量
#include <ctype.h>   // 包含 isdigit() 函数
#include <stdio.h>   // 包含文件操作的标准输入输出函数
#include <stdlib.h>  // 包含标准库函数
#include <string.h>  // 包含字符串处理函数
#include <unistd.h>  // 包含 POSIX 系统调用接口
#include <getopt.h> 

#define MAX_PROCESSES 32678

#ifndef DT_DIR
#define DT_DIR 4  // 通常4是目录类型的标识符
#endif

int show_pids = 0;
int numeric_sort = 0;
#define VERSION "1.0";

typedef struct {
    int pid;
    int ppid;
    char name[256];
} Process;

Process processes[MAX_PROCESSES];
int process_count = 0;

void read_process_info(const char *pid) {
    char path[256];
    FILE *file;
    Process process;

    snprintf(path, sizeof(path), "/proc/%s/stat", pid);
    file = fopen(path, "r");
    if (file) {
        fscanf(file, "%d %s %*c %d", &process.pid, process.name, &process.ppid);
        fclose(file);

        char *start = strchr(process.name, '(');
        char *end = strchr(process.name, ')');

        if (start && end) {
            *end = '\0';
            memmove(process.name, start + 1, strlen(start));
        }

        processes[process_count ++] = process;
    }
}

void read_processes() {
    DIR *dir = opendir("/proc");
    struct dirent *entry;

    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR) {
                if (isdigit(entry->d_name[0])) {
                    read_process_info(entry->d_name);
                }
            }
        }
        closedir(dir);
    }
}

void print_process_tree(int ppid, int level) {
    for (int i = 0; i < process_count; i ++ ) {
        if (processes[i].ppid == ppid) {
            for (int j = 0; j < level; j ++ ) {
                printf("| ");
            }
            if (level > 1) {
                printf("+");
                if (show_pids) {
                    printf("-");
                }
            }
            printf("%s", processes[i].name);
            if (show_pids) {
                printf("(%d)", processes[i].pid);
            }
            printf("\n");
            print_process_tree(processes[i].pid, level + 1);
        }
        
    }
}
int compare_processes(const void *a, const void *b) {
    Process *process_a = (Process *)a;
    Process *process_b = (Process *)b;
    return process_a->pid - process_b->pid;
}

int main(int argc, char *argv[]) {
    int opt;
    struct option long_options[] = {
        {"show-pids", no_argument, 0, 'p'}, 
        {"numeric-sort", no_argument, 0, 'n'},
        {"version", no_argument, 0, 'V'},
    };
    while ((opt = getopt_long(argc, argv, "pnv", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                show_pids = 1;
                break;
            case 'n':
                numeric_sort = 1;
                break;
            case 'v':
                printf("pstree version: ");
                printf("1.0 \n");
                return 0;
            default: 
                fprintf(stderr, "Usage: %s [-p] [--show-pids] [-n] [--numeric-sort] [-V version] [--version version]\n", argv[0]);
                return 1;

        }
    }
    read_processes();
    if (numeric_sort) {
        // 按照PID排序
        qsort(processes, process_count, sizeof(Process), compare_processes);
    }
    print_process_tree(1, 0);
    return 0;
}