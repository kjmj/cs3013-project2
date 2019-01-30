#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>

unsigned long **sys_call_table; // Location of the syscall table in memory

// Used to store references to the original systems calls
asmlinkage long (*ref_sys_open)(const char *pathname, int flags, mode_t mode);
asmlinkage long (*ref_sys_close)(int fd);
asmlinkage long (*ref_sys_read)(int fd, void *buf, size_t count);
asmlinkage long (*ref_sys_cs3013_syscall1)(void);

// Our new system call functions that we will use to intercept the originals

/**
 * Print to the kernel if a normal user (not root) has opened a file
 */
asmlinkage long new_sys_open(const char *pathname, int flags, mode_t mode) {
    int uid = current_uid().val;

    // ignore the root user
    if (uid >= 1000) {
        printk(KERN_INFO
        "User %d is opening file: %s\n", uid, pathname);
    }

    return ref_sys_open(pathname, flags, mode);
}

/**
 * Print to the kernel if a normal user (not root) has closed a file
 */
asmlinkage long new_sys_close(int fd) {
    int uid = current_uid().val;

    // ignore the root user
    if (uid >= 1000) {
        printk(KERN_INFO
        "User %d is closing file descriptor: %d\n", uid, fd);
    }

    return ref_sys_close(fd);
}

/**
 * Print to the kernel if a normal user (not root) has read a file containing the
 * string "VIRUS". This can be seen as the most basic form of an antivirus
 */
asmlinkage long new_sys_read(int fd, void *buf, size_t count) {
    int uid = current_uid().val;
    int fileSize = ref_sys_read(fd, buf, count);

    // if it is not a normal user, just return
    if (uid < 1000) {
        return fileSize;
    }

    // if we find the string "VIRUS" in the file we are reading
    if (strstr((char *) buf, "VIRUS") != NULL) {
        printk(KERN_INFO
        "User %d read from file descriptor %d, but that read contained malicious code!\n", uid, fd);
    }

    return fileSize;
}

/**
 * A sample function that just intercepts a custom system call we added in project 0
 * @return
 */
asmlinkage long new_sys_cs3013_syscall1(void) {
    printk(KERN_INFO "\"'Hello world?!' More like 'Goodbye, world!' EXTERMINATE!\" -- Dalek");
    return 0;
}

/**
 * Find the syscall (system call) table in memory
 * @return The address of the syscall table
 */
static unsigned long **find_sys_call_table(void) {
    unsigned long int offset = PAGE_OFFSET;
    unsigned long **sct;

    while (offset < ULLONG_MAX) {
        sct = (unsigned long **) offset;

        if (sct[__NR_close] == (unsigned long *) sys_close) {
            printk(KERN_INFO
            "Interceptor: Found syscall table at address: 0x%02lX",
                    (unsigned long) sct);
            return sct;
        }

        offset += sizeof(void *);
    }

    return NULL;
}

/**
 * Disable protection for read only memory on the processor, allowing us to
 * overwrite the syscall table
 *
 */
static void disable_page_protection(void) {
    /*
      Control Register 0 (cr0) governs how the CPU operates.

      Bit #16, if set, prevents the CPU from writing to memory marked as
      read only. Well, our system call table meets that description.
      But, we can simply turn off this bit in cr0 to allow us to make
      changes. We read in the current value of the register (32 or 64
      bits wide), and AND that with a value where all bits are 0 except
      the 16th bit (using a negation operation), causing the write_cr0
      value to have the 16th bit cleared (with all other bits staying
      the same. We will thus be able to write to the protected memory.

      It's good to be the kernel!
    */
    write_cr0(read_cr0() & (~0x10000));
}

/**
 * Enable protection for read only memory on the processor
 */
static void enable_page_protection(void) {
    /*
     See the above description for cr0. Here, we use an OR to set the
     16th bit to re-enable write protection on the CPU.
    */
    write_cr0(read_cr0() | 0x10000);
}

/**
 * Initialize the interception of the specified system calls
 */
static int __init

interceptor_start(void) {
    // Find the system call table
    if (!(sys_call_table = find_sys_call_table())) {
        // Cancel the module loading step
        return -1;
    }

    // Store a copy of all the existing functions
    ref_sys_open = (void *) sys_call_table[__NR_open];
    ref_sys_close = (void *) sys_call_table[__NR_close];
    ref_sys_read = (void *)sys_call_table[__NR_read];

    // Replace the existing system calls
    disable_page_protection();

    sys_call_table[__NR_open] = (unsigned long *) new_sys_open;
    sys_call_table[__NR_close] = (unsigned long *) new_sys_close;
    sys_call_table[__NR_read] = (unsigned long *)new_sys_read;
    sys_call_table[__NR_cs3013_syscall1] = (unsigned long *)new_sys_cs3013_syscall1;

    enable_page_protection();

    // And indicate the load was successful
    printk(KERN_INFO
    "Loaded interceptor!");

    return 0;
}

/**
 * Reverts the changes made by the interceptor_start function
 */
static void __exit

interceptor_end(void) {
    // If we don't know what the syscall table is, don't bother
    if (!sys_call_table)
        return;

    // Revert all system calls to what they were before we began
    disable_page_protection();

    sys_call_table[__NR_open] = (unsigned long *) ref_sys_open;
    sys_call_table[__NR_close] = (unsigned long *) ref_sys_close;
    sys_call_table[__NR_read] = (unsigned long *) ref_sys_read;
    sys_call_table[__NR_cs3013_syscall1] = (unsigned long *)ref_sys_cs3013_syscall1;


    enable_page_protection();

    printk(KERN_INFO
    "Unloaded interceptor!");
}

MODULE_LICENSE("GPL");
module_init(interceptor_start);
module_exit(interceptor_end);