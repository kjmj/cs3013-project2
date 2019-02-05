#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

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
    if(argc < 2){
        printf("Please enter a valid PID as an argument\n");
        exit(1);
    }

    unsigned short pid =  atoi(argv[1]);
    struct ancestry *ancestors = (struct ancestry *) malloc(sizeof(struct ancestry));

    long t = testCall2(&pid, ancestors);

    printf("The return values of the system calls are:\n");
    printf("\tcs3013_syscall2: %ld\n", t);

    if(t == -1) {
        printf("Error: PID %hu is not a running process.\n", pid);
        exit(1);
    }

    return 0;
}