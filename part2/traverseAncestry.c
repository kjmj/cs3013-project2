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
 * This function gathers information about the ancestry of the target_pid such as parents, siblings, and children.
 * It will start at target_pid and traverse the ancestry tree all the way until init. It will log information about
 * parents, siblings, and children along the way. Additionally, it will fill the preallocated struct called response
 * with this information.
 *
 * @param target_pid The process id in question
 * @param response A preallocated struct that we will fill with information about the PIDs ancestry
 * @return
 */
asmlinkage long new_sys_cs3013_syscall2(unsigned short *target_pid, struct ancestry *response) {
    /**
     * TODO store info about the ancestors in our struct
     * TODO return this struct to the user space
     */

    // we use this struct to navigate up to the init process
    struct task_struct *task;

    // we use these structs to traverse the siblings and children of each "task"
    struct task_struct *member;
    struct list_head *pos;
    int i; // used to keep track of the number of siblings/children

    unsigned short pid; // the pid to start ancestor traversal at
    struct ancestry ancestryVals; // will eventually contain info about the ancestors of pid

    // make sure pid pointer is valid
    if (copy_from_user(&pid, target_pid, sizeof(unsigned short)) != 0) {
        printk(KERN_INFO "Error reading pid from user space\n");
        return EFAULT;
    }


    // make sure ancestry struct is valid
    if (copy_from_user(&ancestryVals, response, sizeof(struct ancestry)) != 0){
        printk(KERN_INFO "Error reading ancestry struct from user space\n");
        return EFAULT;
    }


    // for every task up to init, starting at the pid we recieved
    for (task = pid_task(find_vpid(pid), PIDTYPE_PID); task != &init_task; task = task->parent) {
        printk(KERN_INFO "At Process: %s [%d] Parent: [%d]\n", task->comm, task->pid, task->parent->pid);

        // Iterate through the siblings of this process
        i = 1;
        list_for_each(pos, &task->sibling) {
            member = list_entry(pos, struct task_struct, sibling);

            // skip ourself (we aren't our own sibling)
            if (member->pid != 0) {
                printk(KERN_INFO "Sibling Process #%d: %s [%d] \n", i, member->comm, member->pid);
                i++;
            }
        }

        // Iterate through the children of this process
        i = 1;
        list_for_each(pos, &task->children) {
            member = list_entry(pos, struct task_struct, sibling);
            printk(KERN_INFO "Child Process #%d: %s [%d] \n", i, member->comm, member->pid);
            i++;
        }
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
