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

int main () {
    unsigned short pid =  getpid(); // TODO get the PID from a command line argument
    struct ancestry *a;

    printf("The return values of the system calls are:\n");
    printf("\tcs3013_syscall2: %ld\n", testCall2(&pid, a));
    return 0;
}