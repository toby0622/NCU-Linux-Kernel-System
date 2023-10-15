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

    printk("pgd_val = 0x%lx, pgd_index = %lu\n", pgd_val(*pgd), pgd_index(virtual_address));

    if (pgd_none(*pgd)) {
        printk("map error: page global directory (pgd)\n");
        return (-1);
    }

    // L2: page p4 directory
    p4d = p4d_offset(pgd, virtual_address);

    printk("p4d_val = 0x%lx, p4d_index = %lu\n", p4d_val(*p4d), p4d_index(virtual_address));

    if (p4d_none(*p4d)) {
        printk("map error: page p4 directory (p4d)\n");
        return (-1);
    }

    // L3: page upper directory
    pud = pud_offset(p4d, virtual_address);

    printk("pud_val = 0x%lx, pud_index = %lu\n", pud_val(*pud), pud_index(virtual_address));

    if (pud_none(*pud)) {
        printk("map error: page upper directory (pud)\n");
        return (-1);
    }

    // L4: page middle directory
    pmd = pmd_offset(pud, virtual_address);

    printk("pmd_val = 0x%lx, pmd_index = %lu\n", pmd_val(*pmd), pmd_index(virtual_address));

    if (pmd_none(*pmd)) {
        printk("map error: page middle directory (pmd)\n");
        return (-1);
    }

    // L5: page table entry
    pte = pte_offset_map(pmd, virtual_address);

    printk("pte_val = 0x%lx, ptd_index = %lu\n", pte_val(*pte), pte_index(virtual_address));

    if (pte_none(*pte)) {
        printk("map error: page table entry (pte)\n");
        return (-1);
    }

    struct page *result_page = pte_page(*pte);

    page_address = page_to_phys(result_page);
    page_offset = virtual_address & ~PAGE_MASK;
    physical_address = page_address | page_offset;

    printk("page_address = %lx, page_offset = %lx\n", page_address, page_offset);
    printk("virtual_address = %lx, physical_address = %lx\n", virtual_address, physical_address);

    return physical_address;
}

SYSCALL_DEFINE4(my_get_physical_addresses, unsigned long*, initial, int, len_vir, unsigned long*, result, int, len_phy) {
    unsigned long virtual_address[len_vir];
    unsigned long physical_address[len_vir];

    long vir_copy = copy_from_user(virtual_address, initial, sizeof(unsigned long)*len_vir);

    printk("len_vir = %d", len_vir);

    for (i = 0; i < len_vir; i++) {
        printk("i = %d", i);
        physical_address[i] = ADDRESS_TRANSFORM(virtual_address[i]);
    }

    long phy_copy = copy_to_user(result, physical_address, sizeof(unsigned long)*len_vir);
    long phy_len_copy = copy_to_user(&len_phy, &len_vir, sizeof(int));    

    return 0;
}