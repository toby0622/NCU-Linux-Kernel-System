#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <syscall.h>

#define TOTAL_ITERATION_NUM  100000000
#define NUM_OF_PRIORITIES_TESTED  40

int my_set_process_priority(int priority) {
    // call sys_my_set_process_priority system call(336)
    return syscall(336, priority);
}

int main() {
    int index=0;
    int priority, i;
    struct timeval start[NUM_OF_PRIORITIES_TESTED], end[NUM_OF_PRIORITIES_TESTED];

    gettimeofday(&start[index], NULL);           // begin
    for(i=1; i<=TOTAL_ITERATION_NUM; i++)
        rand();
    gettimeofday(&end[index], NULL);             // end

    /*================================================================================*/

    for(index=1, priority=101; priority<=139; ++priority, ++index)
    {
        if(!my_set_process_priority(priority))
            printf("Cannot set priority %d.\n", priority);  
                      
        gettimeofday(&start[index], NULL);           // begin
        for(i=1; i<=TOTAL_ITERATION_NUM; i++)
            rand();
        gettimeofday(&end[index], NULL);             // end                       
    }

    /*================================================================================*/

    printf("The process spent %ld microseconds to execute when priority is not adjusted.\n", ((end[0].tv_sec * 1000000 + end[0].tv_usec) - (start[0].tv_sec * 1000000 + start[0].tv_usec)));

    for(i=1; i<=NUM_OF_PRIORITIES_TESTED-1; i++)
        printf("The process spent %ld microseconds to execute when priority is %d.\n", ((end[i].tv_sec * 1000000 + end[i].tv_usec) - (start[i].tv_sec * 1000000 + start[i].tv_usec)), i+100);

    return 0;
}
