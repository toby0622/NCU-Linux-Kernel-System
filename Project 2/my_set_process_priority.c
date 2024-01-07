#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>

SYSCALL_DEFINE1(my_set_process_priority, int, priority){
    // Obtain the current task_struct and its sched_entity
    struct task_struct *task = current;
    struct sched_entity *se = &task->se;
    
    // Check if the provided priority is within the valid range of 101 to 139.
    // If not, return 0 indicating failure.
    if (priority < 101 || priority > 139) {
        return 0;
    }

    // Set the my_fixed_priority field of the task_struct to the provided priority.
    task->my_fixed_priority = priority;

    // Trigger schedule
    schedule();

    // Print process name, process ID, the set priority, and static priority
    printk(KERN_INFO "Process %s (pid=%d) priority set to %d, static_prio %d, Nice Value %d, vruntime %llu\n", task->comm, task->pid, task->my_fixed_priority, task->static_prio, task_nice(task), se->vruntime);

    // Return 1 indicating success.
    return 1;
}
