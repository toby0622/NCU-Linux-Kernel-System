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


* Debug


---

## SYSCALL 設計

* 5-Level Paging　(自 Linux Kernel 4.11 後，Paging 改為 5-Level Structure 來滿足增加的記憶體容量)

<https://www.kernel.org/doc/html/next/x86/x86_64/5level-paging.html>

![image](https://github.com/toby0622/NCU-Linux-Kernel-System/assets/52705034/d53b22d2-5f28-4f89-b056-1b99e71c94c8)

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
