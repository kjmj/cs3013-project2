#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<unistd.h>

// These values MUST match the unistd_32.h modifications:
#define __NR_cs3013_syscall2 378

struct ancestry {
    pid_t ancestors[10];
    pid_t siblings[100];
    pid_t children[100];
};

long testCall2 (unsigned short *target_pid, struct ancestry *response) {
    return (long) syscall(__NR_cs3013_syscall2, target_pid, response);
}

int main (int argc, char* argv[]) {
    pid_t ppid = getpid();
    pid_t cpid = fork();

    if (cpid > 0) {
        waitpid(cpid, NULL, 0);
    } else if (cpid == 0) {
        printf("Forked Parent with PID [%d] into Child with PID: [%d]\n\n", ppid, getpid());

        unsigned short pid = getpid();
        struct ancestry *ancestors = (struct ancestry *) malloc(sizeof(struct ancestry));

        // initialize our struct values to a non-valid pid value
        for(int i = 0; i < 100; i++) {
            if(i < 10) {
                ancestors->ancestors[i] = -1;
            }
            ancestors->siblings[i] = -1;
            ancestors->children[i] = -1;
        }

        long t = testCall2(&pid, ancestors);

        printf("The return values of the system calls are:\n");
        printf("\tcs3013_syscall2: %ld\n\n", t);

        if(t == -1) {
            printf("Error: PID %hu is not a running process.\n", pid);
            exit(1);
        }

        /**
         * Iterate through our results received from kernel space
         */
        printf("Target Process: [%hu]\n", pid);

        for(int i = 0; i < 100; i++) {
            if(ancestors->siblings[i] != -1) {
                printf("Sibling #%d of the Target Process [%d]: [%d]\n", i + 1, pid, ancestors->siblings[i]);
            }
        }

        for(int i = 0; i < 100; i++) {
            if(ancestors->children[i] != -1) {
                printf("Child #%d of the Target Process [%d]: [%d]\n", i + 1, pid, ancestors->children[i]);
            }
        }

        for(int i = 0; i < 10; i++) {
            if(ancestors->ancestors[i] != -1) {
                printf("Ancestor #%d of the Target Process [%d]: [%d]\n", i + 1, pid, ancestors->ancestors[i]);

            }
        }
    } else {
        printf("Error, failed to fork\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}