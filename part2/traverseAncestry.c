#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>

unsigned long **sys_call_table; // Location of the syscall table in memory

struct ancestry {
    pid_t ancestors[10];
    pid_t siblings[100];
    pid_t children[100];
};

// Used to store references to the original systems calls
asmlinkage long (*ref_sys_cs3013_syscall2)(void);

/**
 * This function gathers the siblings, children, and ancestors of target_pid.
 * It logs this information to syslog, and also fills the response struct with this information
 *
 * @param target_pid The process id we want ancestry information for
 * @param response A preallocated struct that we will fill with information about the PIDs ancestry
 * @return 0 on success, -1 if that PID is not a running process
 */
asmlinkage long new_sys_cs3013_syscall2(unsigned short *target_pid, struct ancestry *response) {
    struct task_struct *task; // used to navigate through the processes parents
    struct task_struct *member; // used to navigate through the siblings and children of the process
    int i; // count how many siblings, children, or ancestors we have iterated through

    unsigned short pid; // the pid to start ancestor traversal at
    struct ancestry aTmp;
    struct ancestry *ancestryVals = &aTmp; // will eventually contain info about the ancestors of pid

    // copy pid pointer from user space and make sure it is valid
    if (copy_from_user(&pid, target_pid, sizeof(unsigned short)) != 0) {
        printk(KERN_INFO "Error reading pid from user space\n");
        return EFAULT;
    }

    // copy ancestry struct from user space and make sure it is valid
    if (copy_from_user(ancestryVals, response, sizeof(struct ancestry)) != 0){
        printk(KERN_INFO "Error reading ancestry struct from user space\n");
        return EFAULT;
    }

    // try to find that process
    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if(task == NULL) {
        printk(KERN_INFO "Error: PID %hu is not a running process.\n", pid);
        return -1;
    }

    printk(KERN_INFO "Target Process: [%d]\n", task->pid);

    // Get Siblings
    i = 0;
    list_for_each_entry(member, &(task->sibling), sibling) {
        if (member->pid != 0) {
            ancestryVals->siblings[i] = member->pid;
            i++;
            printk(KERN_INFO "Sibling #%d of Target Process [%d]: [%d] \n", i, pid, member->pid);
        }
    }

    // Get Children
    i = 0;
    list_for_each_entry(member, &(task->children), sibling) {
        ancestryVals->children[i] = member->pid;
        i++;
        printk(KERN_INFO "Child #%d of Target Process [%d]: [%d] \n", i, pid, member->pid);
    }

    // Get Ancestors
    i = 0;
    for (task = task->parent; task->pid >= 0; task = task->parent) {
        ancestryVals->ancestors[i] = task->pid;
        i++;
        printk(KERN_INFO "Ancestor #%d of Target Process [%d]: [%d]\n", i, pid, task->pid);

        // if we have reached init, break out of this loop
        if(task->pid == 0) {
            break;
        }
    }


    //copy over and test that the ancestry pointer is valid
    if (copy_to_user(response, ancestryVals, sizeof(struct ancestry)) != 0){
        printk(KERN_INFO "Error copying ancestry struct to user space\n");
        return EFAULT;
    }

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
            printk(KERN_INFO "Interceptor: Found syscall table at address: 0x%02lX\n", (unsigned long) sct);
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
static int __init interceptor_start(void) {
    // Find the system call table
    if (!(sys_call_table = find_sys_call_table())) {
        // Cancel the module loading step
        return -1;
    }

    // Store a copy of the existing function
    ref_sys_cs3013_syscall2 = (void *) sys_call_table[__NR_cs3013_syscall2];

    // Replace the existing system call
    disable_page_protection();

    sys_call_table[__NR_cs3013_syscall2] = (unsigned long *) new_sys_cs3013_syscall2;

    enable_page_protection();

    // And indicate the load was successful
    printk(KERN_INFO "Loaded interceptor!\n");

    return 0;
}

/**
 * Reverts the changes made by the interceptor_start function
 */
static void __exit interceptor_end(void) {
    // If we don't know what the syscall table is, don't bother
    if (!sys_call_table)
        return;

    // Revert the system call to what it was before we began
    disable_page_protection();

    sys_call_table[__NR_cs3013_syscall2] = (unsigned long *) ref_sys_cs3013_syscall2;

    enable_page_protection();

    printk(KERN_INFO "Unloaded interceptor!\n");
}

MODULE_LICENSE("GPL");
module_init(interceptor_start);
module_exit(interceptor_end);
