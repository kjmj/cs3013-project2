#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/list.h>

/**
 * Traverse the ancestry tree starting at current process and going all the way to init
 * Print information about the siblings and children of each process along the way
 */
static int __init traverseAncestry(void) {
    // we use this struct to navigate up to the init process
    struct task_struct *task;

    // we use these structs to traverse the siblings and children of each "task"
    struct task_struct *member;
    struct list_head *pos;
    int i; // used to keep track of the number of siblings/children

    // for every task up to init
    for (task = current; task != &init_task; task = task->parent) {
        printk(KERN_INFO "Climbed Hierarchy: %s [%d] Parent: [%d]\n", task->comm, task->pid, task->parent->pid);

        // Iterate through the siblings of this process
        i = 1;
        list_for_each(pos, &task->sibling)
        {
            member = list_entry(pos, struct task_struct, sibling);
//            printk("Sibling #%d: %s [%d] Last Schedule: %llu Index of current stored address in ret_stack: %d\n", i, member->comm, member->pid, member->ftrace_timestamp, member->curr_ret_stack);

            // skip ourself as a sibling
            if (member->pid != 0) {
                printk(KERN_INFO "Sibling #%d: %s [%d] \n", i, member->comm, member->pid);
                i++;
            }
        }

        // Iterate through the children of this process
        i = 1;
        list_for_each(pos, &task->children)
        {
            member = list_entry(pos, struct task_struct, sibling);
            printk(KERN_INFO "Child #%d: %s [%d] \n", i, member->comm, member->pid);
            i++;
        }
    }

    return 0;
}

static void __exit cleanup(void) {
    printk(KERN_INFO "Cleaning up module.\n");
}

MODULE_LICENSE("GPL");
module_init(traverseAncestry);
module_exit(cleanup);
