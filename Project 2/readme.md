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

1. System Call 建立在資料夾 `mysyscall` 內
2. 建立 `my_set_process_priority.c`
3. Folder `mysyscall` 內自行新增的 `Makefile` 加入 `my_set_process_priority.o`
4. Linux Source Code 內 Kernel `Makefile` 路徑加上 `mysyscall`
5. System Call Table 新增 System Call Number
6. System Call Header File 新增 System Call Prototype

> [!TIP]
> 詳細步驟參照於 Linux Project 1 「環境架設」欄目，此處不再贅述  
> https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%201/readme.md

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

想法一為在 System Call 將 current task 的 `my_fixed_priority` 欄位設為 priority 的值（101-139）後，手動呼叫 `scheduld()`，而在 `schedule()` 內，加入判斷條件，當 current task 的 priority 為 101-139 且 current task 非 Process 0 時，將其 `static_prio` 欄位調整為 `my_fixed_priority` 欄位的值，在第一次做 System Call 時，就將其 `static_prio` 設定好。

想法二為在 System Call 將 current task 的 `my_fixed_priority` 欄位設為 priority 的值（101-139）後，手動呼叫 `scheduld()`，而在 `schedule()` 內，加入判斷條件，在 Operating System 找到 next task 後且尚未進行 Context Switch 前，判斷當 next task 的 priority 為 101-139 且 next task 非 Process 0 時，將其 `static_prio` 欄位調整為 `my_fixed_priority` 欄位的值。

透過想法一和想法二的測試，經過使用 `printk` 印出參數，可以發現到調整 `static_prio` 後，`nice` 與 `vruntime` 值也會一併被調整。深入分析幾個關鍵參數 `static_prio`、`nice`、`vruntime`，`task_struct` 資料結構中 `static_prio` 是靜態優先順序，不會隨著時間改變，`nice` 值為 `static_prio` - 120，也就是實際值會落在 (-19) 到 20 這個區間，控制進程的優先等級，`nice` 值越低，則進程可以獲得的 CPU 時間越長，`vruntime` 是進程虛擬的執行時間，`nice` 值越小，`vruntime` 速率越慢，如果用一個例子來描述的話就像是一個走得較慢的時鐘，使得進程能夠在固定時間內執行更長時間，而 nice 值較高的進程則像是一個走得較快的時鐘，使得它們的 `vruntime` 增加得更快，從而減少了在 CPU 上的運行時間。

CFS（Completely Fair Scheduler）是 Linux 核心中用於進程調度的主要機制，其核心目標是確保所有進程能夠公平地分享 CPU 資源。在 CFS 的運作中，`vruntime`（虛擬運行時間）佔有非常重要的地位，它代表了進程自啟動以來在 CPU 上運行的時間。CFS 通過比較各進程的 `vruntime` 來決定哪個進程應該獲得下一個 CPU 時間段。當一個進程完成其分配的時間片後，CFS 會更新該進程的 `vruntime`，以反映其在 CPU 上的總運行時間。這個更新過程是通過將進程重新插入到紅黑樹（RB-Tree）的數據結構中來完成的，紅黑樹使得 CFS 能夠高效地找到下一個擁有最小 `vruntime` 值的進程。

閱讀完上述所提及的各項概念，不難發現一件弔詭的情況，明明 `static_prio`、`nice` 和 `vruntime` 之間的配合會對 OS 和 CFS 產生顯著的影響，為何實際實驗出來的結果卻是 `static_prio` 被調整前後，進程執行的時間皆未發生顯著變化？我們對此做了一些探討，經過研究後最終提出的看法或許會讓人大跌眼鏡，就是 CFS（Completely Fair Scheduler）並沒有想像中公平（Fair）！

為了解釋 CFS 為何並不公平，我們需要討論在新版本 Linux Kernel 中所引進的 `task_group`。何謂 `task_group`？`task_group` 是一種系統機制，允許系統將相關的進程組織成群組，並作為「一個單位」進行調度，使得群組內的 Process 在 CPU 時間分配上共享「相同」的調度特性和資源限制。`task_group` 同樣具有權重機制，可以對進程的 `vruntime` 增長速度產生影響，每個 `task_group` 都有一個權重，該權重決定了該群組內進程的 `vruntime` 增長速度。如果一個 `task_group` 的權重較高，其內部進程的 `vruntime` 增長較慢，這意味著這些 Process 可以獲得更多的 CPU 時間；反之，如果一個 `task_group` 的權重較低，其內部進程的 `vruntime` 增長較快，這意味著這些 Process 只能獲得較少 CPU 時間。

不過這就是我們認為的問題所在，由於 `task_group` 是採用 Group 的方式來對內部擁有的所有 Process 權重來進行調整，這種權重機制有可能導致特定群組相對於其他群組或是個別進程獲得更多的 CPU 資源，從而影響到系統中其他進程的公平性。換句話說，擁有高權重的 Group 在進行排程時可能會被 CFS 優先排入隊列，進而占用大量的 CPU 時間，而不是單純只考量各 Process 之間的權重和優先級。

在這樣的前提下，我們猜測調整 `static_prio` 卻無法獲得差異的原因可能有下列兩種：其一，假設有一個高權重的 Group，而我們的 Process 是該 Group 的其中一員，即便我們對該特定 Process 調整其 `static_prio` 的值，由於 CFS 還是先以 `task_group` 所持有的權重來進行排程，整個 Group 在系統中依然擁有優先地位，該特定 Process 的權重差異變成僅能體現在 Group 內部的排程先後，就系統宏觀來看執行的差異非常小；其二則是，如果該 Process 並不在擁有高權重的 Group 內，不論是被分配到其他 Group 還是沒有被分配，不管該特定 Process 調整成甚麼優先級，該 Process 終歸要在高權重的 Group Process 被 CFS 排程完後才能獲取所需 CPU 時間，也就很難看出調整後的差異了。

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
3. 四種任務優先度之間的差異：<https://hackmd.io/@kurokuo/r1naFPjRV>
4. Linux 的进程优先级 NI 和 PR：<https://zhuanlan.zhihu.com/p/196809034>
5. Linux 核心設計不只挑選任務的排程器：<https://hackmd.io/@sysprog/linux-scheduler>
6. Linux 讀書會：<https://hackmd.io/@combo-tw/Linux-%E8%AE%80%E6%9B%B8%E6%9C%83/%2F%40combo-tw%2FHyJXuuy8H>
