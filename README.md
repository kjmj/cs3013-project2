# cs3013-project2

## Part 1
This project demonstrates the interception of system calls in linux. We wrote code to intercept the open, close, and
read system calls.

The modified "open" system call now writes to the log if a user opens a file
The modified "close" system call now writes to the log if a user closes a file
The modified "read" system call now writes to the log if a user reads a file containing the string "VIRUS"

## Part 2
This project traverses the family tree of a PID. It will gather information about that processes siblings, children, and
ancestors. This information will be printed to the system log, and returned as a struct to user space.

procAncestry.c - This is the user space program. This is where we execute our system call and receive the ancestor struct
from the kernel space program.

loadableKernelModule.c - This is the kernel space program. It takes a pid from procAncestry.c and gets the ancestors, siblings,
and children of that process.
