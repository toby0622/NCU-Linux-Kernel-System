# NCU Linux Kernel System Project 2

## Linux 第三十組

* 資工系碩一 鄧祺文 112522083
* 資工系碩一 林適杰 112522010
* 資工系碩一 林瑞庭 112522063

---

## 開發環境

* Virtual machine: VMWare Workstation 17 Player
* Linux kernel: 5.17.7
* Operation system: Ubuntu 20.04.6 LTS

---

## Project 目標

* Objective: Write a new system call int my_set_process_priority(int x) so that a process P can use this new system call my_set_process_priority(int x) to set the priority of the process as x every time when a context switch (i.e. process switch) transfers CPU to process P.

```c
// prototype of the new system call is as follows:     
int my_set_process_priority(int x)
```

---

## 設計方法

* Add a new field in task_struct
修改task_struct的程式碼，新增`my_fixed_priority`欄位用來存取自行設定的priority的數值，不過要在`randomized_struct_fields_start`及`randomized_struct_fields_end`的中間新增，因為為了增加kernel的安全性，kernel使用了編譯器提供的randomize layout，目的是在編譯期將struct中的欄位排序隨機化，這樣可以對struct中的數據提供一定的保護能力，入侵者無法根據原始碼就能掌握struct中的所有數據位址。

1. `vim include/linux/sched.h`進入sched.h
2. 插入`int				my_fixed_priority;`

![image](https://hackmd.io/_uploads/ByaOBDEOp.png)

* Initialize the new field

因為linux在建立新task時的共同function為copy_process()，故我們在copy_process這個函數內對在task_struct內新增的my_fixed_priority欄位做初始化為0。

1. `vim kernel/fork.c`進入fork.c
2. 插入`p->my_fixed_priority = 0;`

![image](https://hackmd.io/_uploads/HJ1Sjgv_T.png)

* __schedule
因為test.c執行結果在各static_prio下，執行時間並無顯著差異，所以我們在__schedule中多加嘗試，在context switch前的各時間點判斷task是否需要調整static_prio。

#### 1 (對照 test result 1)
在__schedule內，直接先判斷current task(prev)是否需調整static_prio。
1. `vim kernel/sched/core.c`進入core.c
2. 在尚未取得next task前插入以下程式碼判斷prev是否需調整static_prio欄位
    ```
    if(prev->static_prio != 0 && prev->my_fixed_priority >= 101 && prev->my_fixed_priority <= 139)
        prev->static_prio = prev->my_fixed_priority;
    ```
    ![image](https://hackmd.io/_uploads/HkCNGtwua.png)

#### 2 (對照 test result 2)
在__schedule內，得到next task後判斷next task(next)是否需調整static_prio。
1. `vim kernel/sched/core.c`進入core.c
2. 在已取得next task後，執行context switch前，插入以下程式碼判斷next是否需調整static_prio欄位
    ```
    if(next->static_prio != 0 && next->my_fixed_priority >= 101 && next->my_fixed_priority <= 139)
                next->static_prio = next->my_fixed_priority;
    ```
    ![image](https://hackmd.io/_uploads/Sk4ZMtvO6.png)

### New system call
1. system call建立在資料夾mysyscall內
2. 建立`my_set_process_priority.c`
3. syscall內自行新增的Makefile加入`my_set_process_priority.o`
4. linux source code內的Makefile路徑加上syscall
5. 新增system call 代碼
6. system call宣告

my_set_process_priority.c
```
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
```

### Test code
testProject2.c
```
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
```

### Test rusult

#### result 1
testProject2.c執行結果：
![image](https://hackmd.io/_uploads/BykD0NwOT.png)

system call內對重要參數print資訊：
![image](https://hackmd.io/_uploads/H1h_AEP_a.png)

#### result 2
testProject2.c執行結果：
![image](https://hackmd.io/_uploads/ryunkYwua.png)

system call內對重要參數print資訊：
![image](https://hackmd.io/_uploads/r1ks1tPua.png)

### 原因探討
在static_prio被調整前後，進程執行時間皆未發生顯著變化。

做法1為在system call將current task的my_fixed_priority欄位設為priority的值(101~139)後，手動呼叫scheduld()，而在schedule()內，加入判斷條件，當current task的priority為101~139且current task非process 0時，將其static_prio欄位調整為my_fixed_priority欄位的值，在第一次做system call時，就將其static_prio設定好。

做法2為在system call將current task的my_fixed_priority欄位設為priority的值(101~139)後，手動呼叫scheduld()，而在schedule()內，加入判斷條件，在O.S.找到next task後且尚未進行context switch前，判斷當next task的priority為101~139且next task非process 0時，將其static_prio欄位調整為my_fixed_priority欄位的值，

另在測試時經由prink印出參數，發現調整static_prio後，nice與vruntime值也會一併被調整。

需要關注的點有static_prio, nice, vruntime，task_struct資料結構中static_prio是靜態優先順序，不會隨著時間改變，nice值為static_prio - 120 (-19~20)，控制進程的優先等級，nice值越低，則進程可以獲得的cpu時間越長，vruntime是進程虛擬的執行時間，nice值越小，vruntime速率越慢，可以想像成在假設每個進程可以執行的時間都是10秒，走得比較慢的時鐘就可以走更久，走得快的時鐘則走得比較快。

CFS排程器在排程時主要是看vruntime值，將vruntime值最小的task執行後，再考慮其使用cpu的時間重新插入紅黑數中，另外還有task_group的概念，推測~~可能是CFS排程器做到的fair與執行時間並未有直接相關~~?(這段還沒研究，感覺可以做個結尾)

參考資料：https://hackmd.io/@RinHizakura/B18MhT00t

### 筆記
建立多個system call時，自行建立的Makefile內寫法如下，否則在make時會報錯。
![image](https://hackmd.io/_uploads/ry8gUrS_T.png)
