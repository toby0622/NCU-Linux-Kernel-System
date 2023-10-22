# NCU Linux Kernel System Project 1

## Linux 第三十組

* 112522083 資工系碩一 鄧祺文
* 資工系碩一 林瑞庭
* 資工系碩一 林適杰

---

## Project 目標

* Objective 1: Design SYSCALL __"void * my_get_physical_addresses(void *)"__ to Get the Physical Address of a Virtual Address in a Process.
* Objective 2: Design Multi-Thread Program with Three Threads Using the New SYSCALL to Show How Memory Areas are Shared by Threads.

---

## 參考資料

* Kernel Compile

1. <https://www.cnblogs.com/aalan/p/16273585.html>
2. <https://blog.csdn.net/weixin_42915431/article/details/121775873>
3. <https://home.gamer.com.tw/creationDetail.php?sn=5696940>

* Debug

1. <https://blog.csdn.net/qq_36393978/article/details/118157426>
2. <https://blog.csdn.net/qq_36393978/article/details/124274364>
3. Additional Package Installed: dwarves, zstd, synaptics

* System Call Design & Other Reference

1. <https://lwn.net/Articles/604287/>
2. <https://lwn.net/Articles/604515/>
3. <https://www.kernel.org/doc/html/next/x86/x86_64/mm.html>
4. <https://www.kernel.org/doc/html/next/x86/x86_64/5level-paging.html>
5. <https://hackmd.io/@combo-tw/Linux-%E8%AE%80%E6%9B%B8%E6%9C%83/%2F%40combo-tw%2FBJlTwJUABB>
6. <https://blog.csdn.net/qq_30624591/article/details/88544739>
7. <https://zhuanlan.zhihu.com/p/490504522>
8. <https://blog.gtwang.org/programming/pthread-multithreading-programming-in-c-tutorial/>
9. <https://askubuntu.com/questions/1435918/terminal-not-opening-on-ubuntu-22-04-on-virtual-box-7-0-0>
10. <https://stackoverflow.com/questions/41090469/linux-kernel-how-to-get-physical-address-memory-management>

---

## SYSCALL 設計

* 5-Level Paging (自 Linux Kernel 4.11 後，Paging 改為 5-Level Structure 來滿足增加的記憶體容量)

<https://www.kernel.org/doc/html/next/x86/x86_64/5level-paging.html>

* 5-Level Paging 示意圖

![image](https://github.com/toby0622/NCU-Linux-Kernel-System/assets/52705034/d53b22d2-5f28-4f89-b056-1b99e71c94c8)

* 設計思路

我們先在 Kernel Stack 中定義兩個 Array，一個用來存放從 User Space 傳入的 Virtual Address，另一個用來存放對應的 Physical Address，並使用 copy_from_user() 將 User Space 傳入的 Virtual Address 複製到 Virtual Address Array 中。接著利用 Linux 定義的 Page Table Macro (包含 pgd_offset、p4d_offset、pud_offset、pmd_offset、pte_offset_map)，逐步從 Virtual Address 取得對應的 Physical Address。最後使用 copy_to_user() 將 Physical Address Array 中的結果複製回 User Space。

* addresstransform.c

```c
// objective 1: transform virtual address to physical address
// objective 2: design a multi-thread program using the new system call to show memory areas shared by threads

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/init_task.h>
#include <linux/pgtable.h>
#include <linux/delay.h>
#include <asm/io.h>

// paging unit is used for virtual address to physical address transformation
unsigned long ADDRESS_TRANSFORM(unsigned long virtual_address) {
    // linux five level paging structure (first introduced in kernel version 4.11)
    pgd_t* pgd;
    p4d_t* p4d;
    pud_t* pud;
    pmd_t* pmd;
    pte_t* pte;

    unsigned long physical_address = 0;
    unsigned long page_address = 0;
    unsigned long page_offset = 0;

    // L1: page global directory
    pgd = pgd_offset(current -> mm, virtual_address);

    printk("pgd_value = 0x%lx, pgd_index = %lu\n", pgd_val(*pgd), pgd_index(virtual_address));

    if (pgd_none(*pgd)) {
        printk("map error: page global directory (pgd)\n");
        return 0;
    }

    // L2: page p4 directory
    p4d = p4d_offset(pgd, virtual_address);

    printk("p4d_value = 0x%lx, p4d_index = %lu\n", p4d_val(*p4d), p4d_index(virtual_address));

    if (p4d_none(*p4d)) {
        printk("map error: page p4 directory (p4d)\n");
        return 0;
    }

    // L3: page upper directory
    pud = pud_offset(p4d, virtual_address);

    printk("pud_value = 0x%lx, pud_index = %lu\n", pud_val(*pud), pud_index(virtual_address));

    if (pud_none(*pud)) {
        printk("map error: page upper directory (pud)\n");
        return 0;
    }

    // L4: page middle directory
    pmd = pmd_offset(pud, virtual_address);

    printk("pmd_value = 0x%lx, pmd_index = %lu\n", pmd_val(*pmd), pmd_index(virtual_address));

    if (pmd_none(*pmd)) {
        printk("map error: page middle directory (pmd)\n");
        return 0;
    }

    // L5: page table entry
    pte = pte_offset_map(pmd, virtual_address);

    printk("pte_value = 0x%lx, ptd_index = %lu\n", pte_val(*pte), pte_index(virtual_address));

    if (pte_none(*pte)) {
        printk("map error: page table entry (pte)\n");
        return 0;
    }

    struct page *result_page = pte_page(*pte);

    page_address = page_to_phys(result_page);
    page_offset = virtual_address & ~PAGE_MASK;
    physical_address = page_address | page_offset;

    printk("page_address = %lx, page_offset = %lx\n", page_address, page_offset);
    printk("virtual_address = %lx, physical_address = %lx\n", virtual_address, physical_address);

    return physical_address;
}

// syscall definition using self-written function
SYSCALL_DEFINE4(my_get_physical_addresses,
                unsigned long*, initial,
                int, virtual_address_length,
                unsigned long*, result,
                int, physical_address_length) {
    unsigned long virtual_address[virtual_address_length];
    unsigned long physical_address[virtual_address_length];

    // original: unsigned long copy_from_user(void * to, const void __user * from, unsigned long n);
    //
    long copy_virtual_address = copy_from_user(virtual_address, initial, sizeof(unsigned long)*virtual_address_length);

    printk("virtual_address_length = %d", virtual_address_length);

    int i;

    for (i = 0; i < virtual_address_length; i++) {
        printk("i = %d", i);
        physical_address[i] = ADDRESS_TRANSFORM(virtual_address[i]);
    }

    // original: unsigned long copy_to_user(void __user *to, const void *from, unsigned long n);
    long copy_physical_address = copy_to_user(result, physical_address, sizeof(unsigned long)*virtual_address_length);
    long copy_physical_length = copy_to_user(&physical_address_length, &virtual_address_length, sizeof(int));

    return 0;
}
```

---

## Physical Address 結果之驗證

* 驗證方法

利用 Linux 內建之 devmem 功能直接對特定地址進行讀取和修改資料值，並對該特定地址進行監測輸出來確認 Physical Address 的正確性。

<https://zhuanlan.zhihu.com/p/575667017>

* test2.c

```c
// address result checker: should be collaborated with "devmem" function

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <syscall.h>

#define TRUE 1
#define FALSE 0

int bss_value;
int data_value = 12345;

int code_function() {
    return 0;
}

int main() {
    sleep(2);

    int block_input;
    int stack_value = 10;

    unsigned long stack = (unsigned long)&stack_value;
    unsigned long lib = (unsigned long)getpid;
    unsigned long heap = (unsigned long)malloc(sizeof(int));
    unsigned long bss = (unsigned long)&bss_value;
    unsigned long data = (unsigned long)&data_value;
    unsigned long code = (unsigned long)code_function;

    int length = 6;

    unsigned long virtual_address[6] = {stack, lib, heap, bss, data, code};
    unsigned long physical_address[6];

    long copy = syscall(548, virtual_address, length, physical_address, length);
    
    if (copy < 0) {
        printf("transfer error: address invalid");
        exit(1);
    }

    while(TRUE) {
        printf("pid = %d\n", (int)getpid());
        printf("segment\tvalue\tvir_addr\tphy_addr\n");
        printf("stack\t%d\t%lx\t%lx\n", stack_value, virtual_address[0], physical_address[0]);
        printf("getpid\t%d\t%lx\t%lx\n", (int)getpid(), virtual_address[1], physical_address[1]);
        printf("heap\t%d\t%lx\t%lx\n", *(int*)heap, virtual_address[2], physical_address[2]);
        printf("bss\t%d\t%lx\t%lx\n", bss_value, virtual_address[3], physical_address[3]);
        printf("data\t%d\t%lx\t%lx\n", data_value, virtual_address[4], physical_address[4]);
        printf("code\t%d\t%lx\t%lx\n", code_function(), virtual_address[5], physical_address[5]);
        
        printf("Options >> 1: Next, 0: Exit\n");
        scanf("%d", &block_input);
        
        if (block_input == 0) {
            break;
        }
    }

    return 0;
}
```

---

## Multi-Thread Program 的 Memory 共用情況

---
