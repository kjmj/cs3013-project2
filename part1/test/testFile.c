#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

// These values MUST match the unistd_32.h modifications:
#define __NR_cs3013_syscall1 377

long testCall_cs3013_syscall1 (void) {
    return (long) syscall(__NR_cs3013_syscall1);
}

// open the file at path with read only permissions
long testCall_open (char *path) {
    return (long) syscall(__NR_open, path, O_RDONLY);
}

// close the file at file descriptor fd
long testCall_close (int fd) {
    return (long) syscall(__NR_close, fd);
}

// try to read the file at file descriptor fd
long testCall_read (int fd) {
    char buff[1000];
    return (long) syscall(__NR_read, fd, buff, 1000);
}

int main () {
    printf("The return values of the system calls are:\n");
    printf("\tcs3013_syscall1: %ld\n", testCall_cs3013_syscall1());

    // open our test files and note their file descriptors
    int fd1 = testCall_open("withoutVirus.txt");
    int fd2 = testCall_open("withVirus.txt");
    printf("\topen withoutVirus.txt: %d\n", fd1);
    printf("\topen withVirus.txt: %d\n", fd2);

    // try to read a file, both with and without a the "VIRUS" string
    printf("\tread withoutVirus.txt: %ld\n", testCall_read(fd1));
    printf("\tread withVirus.txt: %ld\n", testCall_read(fd2));

    // try to close the files we just opened
    printf("\tclose withoutVirus.txt: %ld\n", testCall_close(fd1));
    printf("\tclose withVirus.txt: %ld\n", testCall_close(fd2));

    return 0;
}