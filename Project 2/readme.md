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

## Linux Kernel 設計調整

* Add New Field in `task_struct`  

修改 `task_struct` 的程式碼，新增 `my_fixed_priority` 欄位用來存取自行設定的 Priority 的數值，不過要在 `randomized_struct_fields_start` 及 `randomized_struct_fields_end` 的中間新增，因為為了增加 Kernel 的安全性，Kernel 使用了 Compiler 所提供的 Randomize Layout，目的是在編譯期將 `struct` 中的欄位排序隨機化，這樣可以對 `struct` 中的數據提供一定的保護能力，入侵者無法根據原始碼就能掌握 `struct` 中的所有數據位址。

> [!IMPORTANT]
> 1. `vim include/linux/sched.h` 進入 `sched.h`
> 2. 插入 `int my_fixed_priority;`

<p align="center">
    <img src="https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%202/Screenshots/1.png?raw=true" alt="P1"/>
</p>

* Initialize New Field

因為 Linux 在建立新 Task 時的共同 Function 為 `copy_process()`，故我們在 `copy_process` 這個函數內對在 `task_struct` 新增的 `my_fixed_priority` 欄位做初始化為 0。

> [!IMPORTANT]
> 1. `vim kernel/fork.c` 進入 `fork.c`
> 2. 插入 `p -> my_fixed_priority = 0;`

<p align="center">
    <img src="https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%202/Screenshots/2.png?raw=true" alt="P2"/>
</p>

* `__schedule`

因為 `test.c` 執行結果在各 `static_prio` 下，執行時間並無顯著差異，所以我們在 `__schedule` 中多加嘗試，在 Context Switch 前的各時間點判斷 Task 是否需要調整 `static_prio`。

> [!NOTE]
> 想法一（對照 Test Result 1）  
> 在 `__schedule` 內，直接先判斷 current task(prev) 是否需調整 `static_prio`。

1. `vim kernel/sched/core.c` 進入 `core.c`
2. 在尚未取得 next task 前插入以下程式碼判斷 prev 是否需調整 `static_prio` 欄位

```c
if (prev -> static_prio != 0 && prev -> my_fixed_priority >= 101 && prev -> my_fixed_priority <= 139)
    prev -> static_prio = prev -> my_fixed_priority;
```
    
<p align="center">
    <img src="https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%202/Screenshots/3.png?raw=true" alt="P3"/>
</p>

> [!NOTE]
> 想法二（對照 Test Result 2）  
> 在 `__schedule` 內，得到 next task 後判斷 next task(next) 是否需調整 `static_prio`。

1. `vim kernel/sched/core.c` 進入 `core.c`
2. 在已取得 next task 後，執行 Context Switch 前，插入以下程式碼判斷 next 是否需調整 `static_prio` 欄位

```c
if (next -> static_prio != 0 && next -> my_fixed_priority >= 101 && next -> my_fixed_priority <= 139)
            next -> static_prio = next -> my_fixed_priority;
```

<p align="center">
    <img src="https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%202/Screenshots/4.png?raw=true" alt="P4"/>
</p>

---

## Linux SYSCALL 建立

1. System Call 建立在資料夾 mysyscall 內
2. 建立 `my_set_process_priority.c`
3. Syscall Folder 內自行新增的 Makefile 加入 `my_set_process_priority.o`
4. Linux Source Code 內的 Makefile 路徑加上 Syscall
5. 新增 System Call 代碼
6. System Call 宣告

* `my_set_process_priority.c`

```c
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

---

## 功能測試

* `testProject2.c`

```c
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

---

## 執行結果

* Test Result 1（對應想法一）

`testProject2.c` 執行結果：

<p align="center">
    <img src="https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%202/Screenshots/5.png?raw=true" alt="P5"/>
</p>

System Call 內對重要參數 Print 資訊：

<p align="center">
    <img src="https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%202/Screenshots/6.png?raw=true" alt="P6"/>
</p>

* Test Result 2（對應想法二）

`testProject2.c` 執行結果：

<p align="center">
    <img src="https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%202/Screenshots/7.png?raw=true" alt="P7"/>
</p>

System Call 內對重要參數 Print 資訊：

<p align="center">
    <img src="https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%202/Screenshots/8.png?raw=true" alt="P8"/>
</p>

---

## 結果分析

> [!NOTE]
> 在 `static_prio` 被調整前後，進程執行時間皆未發生顯著變化。

想法一為在 System Call 將 current task 的 `my_fixed_priority` 欄位設為 priority 的值（101-139）後，手動呼叫 `scheduld()`，而在 `schedule()` 內，加入判斷條件，當 current task 的 priority 為 101-139 且 current task 非 Process 0 時，將其 `static_prio` 欄位調整為 `my_fixed_priority` 欄位的值，在第一次做 System Call時，就將其 `static_prio` 設定好。

想法二為在 System Call 將 current task 的 `my_fixed_priority` 欄位設為 priority 的值（101-139）後，手動呼叫 `scheduld()`，而在 `schedule()` 內，加入判斷條件，在 Operating System 找到 next task 後且尚未進行 Context Switch 前，判斷當 next task 的 priority 為 101-139 且 next task 非 Process 0 時，將其 `static_prio` 欄位調整為 `my_fixed_priority` 欄位的值。

另在測試時經由 `printk` 印出參數，發現調整 `static_prio` 後，`nice` 與 `vruntime` 值也會一併被調整。

需要關注的點有 `static_prio`、`nice`、`vruntime`，`task_struct` 資料結構中 `static_prio` 是靜態優先順序，不會隨著時間改變，`nice` 值為 `static_prio` - 120，也就是實際值會落在 -19-20 這個區間，控制進程的優先等級，`nice` 值越低，則進程可以獲得的 CPU 時間越長，`vruntime` 是進程虛擬的執行時間，`nice` 值越小，`vruntime` 速率越慢，可以想像成在假設每個進程可以執行的時間都是 10 秒，走得比較慢的時鐘就可以走更久，走得快的時鐘則走得比較快。

CFS 排程器在排程時主要是看 `vruntime` 值，將 `vruntime` 值最小的 task 執行後，再考慮其使用 CPU 的時間重新插入紅黑樹中，另外還有 `task_group` 的概念，推測~~可能是CFS排程器做到的fair與執行時間並未有直接相關~~?(這段還沒研究，感覺可以做個結尾)

> [!TIP]
> * nice 值：nice 是一個範圍在 -20 到 +19 的整數，用於表示進程的優先級，數值越低則優先級越高。nice 值影響進程的權重（weight），這是 CFS 用來決定進程應獲得多少 CPU 時間的關鍵因素。  
> * weight 和 nice 值：CFS 根據 nice 計算進程的 weight。nice 值越低則 weight 越高，進程獲得的 CPU 時間越多。weight 的計算公式是基於 nice 值的，並且是非線性的。這意味著 nice 值的微小變化可以導致權重的顯著變化。  
> * vruntime 值：vruntime 是進程的虛擬運行時間，它是 CFS 用來追蹤進程已獲得的 CPU 時間的。每當進程在 CPU 上運行時，其 vruntime 會增加。
> * nice 與 vruntime 之間的關係：進程的 nice 值間接通過影響權重來影響其 vruntime 的增長速度。一個 nice 值較高（優先級較低）的進程會有較低的權重，導致其 vruntime 增長較快，從而減少其獲得 CPU 時間的機會。相反，nice 值較低（優先級較高）的進程會有較高的權重，其 vruntime 增長較慢，從而增加其獲得 CPU 時間的機會。

---

## 錯誤筆記及參考資料

* 錯誤筆記

> [!CAUTION]
> 建立多個 System Call 時，自行建立的 Makefile 內寫法如下，否則在 make 時會報錯。

<p align="center">
    <img src="https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%202/Screenshots/9.png?raw=true" alt="P9"/>
</p>

* 參考資料

1. 概述 CFS Scheduler：<https://hackmd.io/@RinHizakura/B18MhT00t>
2. 深入剖析 CFS Scheduler：<https://hackmd.io/@RinHizakura/BJ9m_qs-5>
