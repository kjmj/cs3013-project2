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
    if (fork() != 0) {
        wait(NULL);
        printf("I am parent and my id is: %d\n", getpid());
    } else {
        printf("Child id: %d\n", getpid());
        unsigned short pid = getpid();
        struct ancestry *ancestors = (struct ancestry *) malloc(sizeof(struct ancestry));
        long r = testCall2(&pid, ancestors);

        printf("The return values of the system calls are:\n");
        printf("\tcs3013_syscall2: %ld\n", r);

        if(r == -1) { // some error in our system call
            exit(1);
        }

        /**
         * Iterate through our ancestor results
         */
        printf("Started at PID: [%hu]\n", pid);

        for(int i = 0; i < 100; i++) {
            if(ancestors->siblings[i] != 0) {
                printf("Sibling #%d PID: [%d]\n", i + 1, ancestors->siblings[i]);
            }
        }

        for(int i = 0; i < 100; i++) {
            if(ancestors->children[i] != 0) {
                printf("Child #%d PID: [%d]\n", i + 1, ancestors->children[i]);
            }
        }

        for(int i = 0; i < 10; i++) {
            if(ancestors->ancestors[i] != 0) {
                printf("Parent #%d: [%d]\n", i + 1, ancestors->ancestors[i]);
            }
        }

        return 0;
    }
    return 0;
}