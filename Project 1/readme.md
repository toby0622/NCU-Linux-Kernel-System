# NCU Linux Kernel System Project 1

## Linux 第三十組

* 112522083 資工系碩一 鄧祺文
* 112522063 資工系碩一 林瑞庭
* 112522010 資工系碩一 林適杰

---

## Project 目標

* Objective 1: Design SYSCALL __"void * my_get_physical_addresses(void *)"__ to Get the Physical Address of a Virtual Address in a Process.
* Objective 2: Design Multi-Thread Program with Three Threads Using the New SYSCALL to Show How Memory Areas are Shared by Threads.

---

## 開發環境

* Virtual Machine: Oracle VM VirtualBox
* Operating System: Ubuntu 20.04 LTS
* Linux Kernel: Kernel Version 5.15.86 -> Kernel Version 5.17.7 After Kernel Compile

---

## 環境架設

* 在架設完成虛擬機後，首先要做的就是進行系統更新

```shell
sudo apt update && sudo apt upgrade -y
```

* 安裝編譯 Linux Kernel 所需的相關 Dependencies，順帶安裝 Vim 用以直接檢視文件

```shell
sudo apt install build-essential libncurses-dev libssl-dev libelf-dev bison flex -y
sudo apt install vim -y
```

* 安裝 Debug Linux Kernel 所需的額外套件

```shell
sudo apt-get install dwarves
sudo apt-get install zstd
sudo apt-get install synaptic
```

* 清除已經安裝的 Packages（可選）

```shell
sudo apt clean && sudo apt autoremove -y
```

* 利用 wget 通過 CDN 下載 Linux Kernel（不推薦 Kernel Version 6，新版本在新增 SYSCALL 上有變動，會多出很多的坑，非必要不用自找麻煩），並將 Source Code 解壓縮

```shell
wget -P ~/ https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.17.7.tar.xz
tar -xvf ~/linux-5.17.7.tar.xz -C ~/
```

* 檢查當前的 Kernel Version（如果是升級 Kernel Version，在安裝完新的 Kernel 後此值會被修改更新）

```shell
uname -r
```

* 進入先前解壓縮完的 Linux Kernel Source Code，並建立新的資料夾用以存放即將設計的 SYSCALL

```shell
cd ~/linux-5.17.7/
mkdir mysyscall
```

* 創建新的 SYSCALL 並添加 SYSCALL 所需的 C Code（詳細代碼撰寫於下一章節「SYSCALL 設計」）

```shell
vim mysyscall/addresstransform.c
```

* 創建新的 SYSCALL 調用之 Makefile 並添加代碼（Makefile 為特殊檔案，用以包含編譯規則）

```shell
vim mysyscall/Makefile
i -> Insert
obj-y := addresstransform.o
:wq -> Write File & Quit Vim
```

* 將 SYSCALL 調用增加至 Kernel 的 Makefile 中（要在core-y    += ... /(後面再加上)mysyscall/）

```shell
vim Makefile
/core-y    += -> Search "core-y    +="
i -> Insert
core-y     += kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ mysyscall/
:wq -> Write File & Quit Vim
```

* 將 SYSCALL 調用增加至 Kernel 的 Function Header 中（使用外部 Function 需要 asmlinkage long 宣告）

```shell
vim include/linux/syscalls.h
i -> Insert
asmlinkage long my_get_physical_addresses(unsigned long* initial, int virtual_address_length, unsigned long* result, int physical_address_length);
:wq -> Write File & Quit Vim
```

* 將 SYSCALL 調用增加至 Kernel 的 SYSTEM_TABLE 中（新增到最底下欄位）

```shell
vim arch/x86/entry/syscalls/syscall_64.tbl
i -> Insert
548     64     my_get_physical_addresses    sys_my_get_physical_addresses
:wq -> Write File & Quit Vim
```

* 創建 Linux Kernel .config 或直接複製舊有的 .config（此處選擇原先系統之 .config）將 /boot/config COPY 到當前位置

```shell
sudo cp -v /boot/config-$(uname -r) .config
```

* 調整 Kernel Compile 的相關參數

若需使用 devmem 相關功能，要於此步驟關閉 Filter access to /dev/mem 選項，該功能才能正常運作（詳細請查看後續章節「Physical Address 結果之驗證」）。
善用'/'搜尋devmem關鍵字，使用空白鍵將[*]取消

```shell
sudo make menuconfig
```

* 調整 Kernel Compile 的驗證金鑰，如果更新出錯使用 :w! 強制寫入再 :wq

```shell
vim .config

/CONFIG_SYSTEM_TRUSTED_KEYS -> Search "CONFIG_SYSTEM_TRUSTED_KEYS"
i -> Insert
CONFIG_SYSTEM_TRUSTED_KEYS=""

/CONFIG_SYSTEM_REVOCATION_KEYS -> Search "CONFIG_SYSTEM_REVOCATION_KEYS"
i -> Insert
CONFIG_SYSTEM_REVOCATION_KEYS=""

:wq -> Write File & Quit Vim
```

* 清理 Kernel Compile 結果（可選，有需要時使用）

```shell
# 清除包含 .config 的編譯文件（曾 Compile 過要完全重新開始）
sudo make mrproper
# 清除不包含 .config 的編譯文件（Compile 發生錯誤要再次嘗試前）
sudo make clean
```

* 編譯安裝 Linux Kernel（nproc 可以查詢當前機器所有的 Logical Cores）

echo $? 用於檢查狀態，若回傳為 0，可以進行下一步；若回傳不為 0，代表有錯誤發生，請去除 -j 平行化參數，僅執行前方 make 指令，並根據 Terminal 的 Error 提示進行排查（使用 -j 參數會平行分配任務加快 Compile 速度，但預設會跳過 Error 繼續往下跑，直到沒辦法 Compile 強制退出為止，導致回傳不為零，這種情況難以進行排查，改回使用 sudo make 會於錯誤地方停下提示）。

```shell
sudo make -j$(nproc)
echo $?
sudo make modules_install -j$(nproc)
echo $?
sudo make install -j$(nproc)
echo $?
```

* 檢查 Kernel 有被正確安裝，於 grub 中確認即可重開機（新版 Ubuntu 預設會把新 Kernel 設為首要，舊版本可能會需要手動配置 Bootloader）

```shell
sudo update-grub
sudo reboot
```

* 重開機後檢查 Kernel Version 是否正確，並撰寫 Tester 來檢查新的 SYSCALL 運作

```shell
uname -r
```

```c
// test1.c

#include <syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#define TRUE 1
#define FALSE 0

int main() {
    int virtual_address_length = 3;
    int a = 0;
    int b = 0;
    int c = 0;

    uintptr_t virtual_address[3] = {&a, &b, &c};

    int i = 0;

    printf("virtual_address_length = %d\n", virtual_address_length);

    for (i = 0; i < virtual_address_length; i++) {
        printf("i = %d; virtual_address = %lx\n", i, virtual_address[i]);
    }

    int physical_address_length = 3;
    uintptr_t physical_address[3];

    int copy = syscall(548, virtual_address, virtual_address_length, physical_address, physical_address_length);

    printf("physical_address_length = %d\n", physical_address_length);
    
    for (i = 0; i < virtual_address_length; i++) {
        printf("i = %d; virtual_address = %lx; physical_address = %lx\n", i, virtual_address[i], physical_address[i]);
    }

    while(TRUE);

    return a;
}
```

---

## SYSCALL 設計

* 5-Level Paging（自 Linux Kernel 4.11 後，Paging 改為 5-Level Structure 來滿足增加的記憶體容量）

<https://www.kernel.org/doc/html/next/x86/x86_64/5level-paging.html>

* 5-Level Paging 示意圖

![image](https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%201/Screenshots/sc1.png)

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

利用 devmem2 功能直接對特定地址進行讀取和修改資料值，並對該特定地址進行監測輸出來確認 Physical Address 的正確性。
（需於 Kernel 編譯時執行 makeconfig，進入 Kernel Hacking 分類，並將限制 devmem 功能之 Filter access to /dev/mem 關閉，才能正常調用）

<https://blog.csdn.net/happen23/article/details/113700200>  
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

* 輸出結果

我們先簡單的將 Code、Data、BSS、Heap 和 Stack 的 Virtual Address 和轉換後的 Physical Address 進行輸出，結果如下圖所示。

![image](https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%201/Screenshots/sc7.png)

* devmem2 檢查（檢查 Data Segment）

為了確認我們轉出來的 Physical Address 是正確的，以 Data Segment 做舉例，當透過 devmem2 直接讀取 0x2046d1010（Data Segment Physical Address）時，我們可以看到他回傳的 Data Value 是 0x3039，也就是我們所給予初始值之 12345（Hex to Dec）。

![image](https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%201/Screenshots/sc8.png)

* devmem 修改 Value（修改 Data Segment）

同時，由於 devmem2 可以直接對特定記憶體位置進行賦值，為了進行二次確認，我們將原先的 12345 改為 54321 重新寫入至 Data 中。

![image](https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%201/Screenshots/sc9.png)

* 驗證成功

回到最開始撰寫的測試代碼，輸入 1 進行新一輪檢測，可以發現 Data 的值已經被更改為 54321，至此 Virtural Address to Physical Address 功能圓滿完成並完成驗證。

![image](https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%201/Screenshots/sc10.png)

---

## Multi-Thread Program 於 Memory 中的分布和共用情況

* 驗證方法

於 Linux 中撰寫一個 Multi-Thread 的測試，並輸出各線程所使用的 Memory Area，以此來分析多線程於 Memory 中的 Segment 分佈與共用情況。

<https://blog.csdn.net/weixin_42250655/article/details/105234980>  
<https://blog.csdn.net/stpeace/article/details/43282611>

* multithread.c

```c
// objective 2: design a multi-thread program using the new system call to show memory areas shared by threads

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>

int bss_value;
int data_value = 123;
int code_function() {
    return 0;
}

pid_t gettid() {
    return syscall(SYS_gettid);
}

// requirement: using __thread to create TLS
static __thread int thread_local_storage_value = 246;

// multi-thread process: t1
void *thread1(void *arg) {
    sleep(2);
    int stack_value = 100;
    unsigned long TLS = (unsigned long)&thread_local_storage_value;
    unsigned long stack = (unsigned long)&stack_value;
    unsigned long lib = (unsigned long)getpid;
    // requirement: using malloc to create heap
    unsigned long heap = (unsigned long)malloc(sizeof(int));
    unsigned long bss = (unsigned long)&bss_value;
    unsigned long data = (unsigned long)&data_value;
    unsigned long code = (unsigned long)code_function;

    int len = 7;
    unsigned long virtual_address[7] = {TLS, stack, lib, heap, bss, data, code};
    unsigned long physical_address[7];

    long copy = syscall(548, virtual_address, len, physical_address, len);

    if (copy < 0) {
        printf("transfer error: virtual to physical");
        exit(1);
    }

    printf("============= thread1 =============\n");
    printf("pid = %d  tid = %d\n", (int)getpid(), (int)gettid());
    printf("segment\tvir_addr\tphy_addr\n");
    printf("TLS\t%lx\t%lx\n", virtual_address[0], physical_address[0]);
    printf("stack\t%lx\t%lx\n", virtual_address[1], physical_address[1]);
    printf("lib\t%lx\t%lx\n", virtual_address[2], physical_address[2]);
    printf("heap\t%lx\t%lx\n", virtual_address[3], physical_address[3]);
    printf("bss\t%lx\t%lx\n", virtual_address[4], physical_address[4]);
    printf("data\t%lx\t%lx\n", virtual_address[5], physical_address[5]);
    printf("code\t%lx\t%lx\n", virtual_address[6], physical_address[6]);

    // child thread process exit
    pthread_exit(NULL);
}

// multi-thread process: t2
void *thread2(void *arg) {
    sleep(4);
    int stack_value = 200;
    unsigned long TLS = (unsigned long)&thread_local_storage_value;
    unsigned long stack = (unsigned long)&stack_value;
    unsigned long lib = (unsigned long)getpid;
    unsigned long heap = (unsigned long)malloc(sizeof(int));
    unsigned long bss = (unsigned long)&bss_value;
    unsigned long data = (unsigned long)&data_value;
    unsigned long code = (unsigned long)code_function;

    int len = 7;
    unsigned long virtual_address[7] = {TLS, stack, lib, heap, bss, data, code};
    unsigned long physical_address[7];

    long copy = syscall(548, virtual_address, len, physical_address, len);

    if (copy < 0) {
        printf("transfer error: virtual to physical");
        exit(1);
    }

    printf("============= thread2 =============\n");
    printf("pid = %d  tid = %d\n", (int)getpid(), (int)gettid());
    printf("segment\tvir_addr\tphy_addr\n");
    printf("TLS\t%lx\t%lx\n", virtual_address[0], physical_address[0]);
    printf("stack\t%lx\t%lx\n", virtual_address[1], physical_address[1]);
    printf("lib\t%lx\t%lx\n", virtual_address[2], physical_address[2]);
    printf("heap\t%lx\t%lx\n", virtual_address[3], physical_address[3]);
    printf("bss\t%lx\t%lx\n", virtual_address[4], physical_address[4]);
    printf("data\t%lx\t%lx\n", virtual_address[5], physical_address[5]);
    printf("code\t%lx\t%lx\n", virtual_address[6], physical_address[6]);

    // child thread process exit
    pthread_exit(NULL);
}

// multi-thread process: t3
void *thread3(void *arg) {
    sleep(6);
    int stack_value = 300;
    unsigned long TLS = (unsigned long)&thread_local_storage_value;
    unsigned long stack = (unsigned long)&stack_value;
    unsigned long lib = (unsigned long)getpid;
    unsigned long heap = (unsigned long)malloc(sizeof(int));
    unsigned long bss = (unsigned long)&bss_value;
    unsigned long data = (unsigned long)&data_value;
    unsigned long code = (unsigned long)code_function;

    int len = 7;
    unsigned long virtual_address[7] = {TLS, stack, lib, heap, bss, data, code};
    unsigned long physical_address[7];

    long copy = syscall(548, virtual_address, len, physical_address, len);

    if (copy < 0) {
        printf("transfer error: virtual to physical");
        exit(1);
    }

    printf("============= thread3 =============\n");
    printf("pid = %d  tid = %d\n", (int)getpid(), (int)gettid());
    printf("segment\tvir_addr\tphy_addr\n");
    printf("TLS\t%lx\t%lx\n", virtual_address[0], physical_address[0]);
    printf("stack\t%lx\t%lx\n", virtual_address[1], physical_address[1]);
    printf("lib\t%lx\t%lx\n", virtual_address[2], physical_address[2]);
    printf("heap\t%lx\t%lx\n", virtual_address[3], physical_address[3]);
    printf("bss\t%lx\t%lx\n", virtual_address[4], physical_address[4]);
    printf("data\t%lx\t%lx\n", virtual_address[5], physical_address[5]);
    printf("code\t%lx\t%lx\n", virtual_address[6], physical_address[6]);

    // child thread process exit
    pthread_exit(NULL);
}

int main() {
    pthread_t t1, t2, t3;

    printf("virtual to physical system call\n");

    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);
    pthread_create(&t3, NULL, thread3, NULL);
    
    sleep(8);
    int stack_value = 10;
    unsigned long TLS = (unsigned long)&thread_local_storage_value;
    unsigned long stack = (unsigned long)&stack_value;
    unsigned long lib = (unsigned long)getpid;
    unsigned long heap = (unsigned long)malloc(sizeof(int));
    unsigned long bss = (unsigned long)&bss_value;
    unsigned long data = (unsigned long)&data_value;
    unsigned long code = (unsigned long)code_function;

    int len = 7;
    unsigned long virtual_address[7] = {TLS, stack, lib, heap, bss, data, code};
    unsigned long physical_address[7];

    long copy = syscall(548, virtual_address, len, physical_address, len);

    if (copy < 0) {
        printf("transfer error: virtual to physical");
        exit(1);
    }

    printf("============== main ==============\n");
    printf("pid = %d  tid = %d\n", (int)getpid(), (int)gettid());
    printf("segment\tvir_addr\tphy_addr\n");
    printf("TLS\t%lx\t%lx\n", virtual_address[0], physical_address[0]);
    printf("stack\t%lx\t%lx\n", virtual_address[1], physical_address[1]);
    printf("lib\t%lx\t%lx\n", virtual_address[2], physical_address[2]);
    printf("heap\t%lx\t%lx\n", virtual_address[3], physical_address[3]);
    printf("bss\t%lx\t%lx\n", virtual_address[4], physical_address[4]);
    printf("data\t%lx\t%lx\n", virtual_address[5], physical_address[5]);
    printf("code\t%lx\t%lx\n", virtual_address[6], physical_address[6]);

    printf("--------- thread address ----------\n");
    printf("t1 = %p\n", &t1);
    printf("t2 = %p\n", &t2);
    printf("t3 = %p\n", &t3);

    return 0;
}
```

* 輸出結果

![image](https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%201/Screenshots/sc12.png)

* Memory 共用情況探討

我們由輸出結果可以發現，執行單一的 Multi-Thread 程式，不同 Thread（thread1、thread2、thread3）之間的 Code、Data、BSS 和 Library 段是共用的，Stack 和 Heap 段則是各自配置了一部分記憶體。

![image](https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%201/Screenshots/sc11.png)

如果前面的部分不夠清楚，我們可以簡易修改 multithread.c，在最後面加上一個 while(TRUE) 來 Block 程式進程，並利用 sudo cat /proc/<pid>/maps 來進行對照，會發現 Stack 和 Heap 確實比較特別，可以看到 [heap] 下面額外配置的空間就是 Thread Heap 的部分。此外，我們還發現 Library 之間有一大堆空白，對照得出該部分即為 Thread Stack。而關於 TLS（Thread Local Storage）則是會基於各 Thread 複製一份到其 Thread Stack 的頂端，至於 Main 的部分則會配置在所有 Thread Stack 的上方。

<https://www.cnblogs.com/arnoldlu/p/10272466.html>

![image](https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%201/Screenshots/sc13.png)

* Multi-Thread 在 Memory 中的堆疊

由前面所進行的分析可以得出 Multi-Thread 在 Memory 中的配置情況應如下圖：

![drawio](https://github.com/toby0622/NCU-Linux-Kernel-System/blob/main/Project%201/Drawio/drawio.png)

---

## 建構中碰到的各式問題

* 編譯 Linux Kernel 報錯：No rule to make target ‘debian/canonical-certs.pem‘, needed by ‘certs/x509_certificate_list‘

> 修改 CONFIG_SYSTEM_TRUSTED_KEYS，將其賦與空值。
> 修改 CONFIG_SYSTEM_REVOCATION_KEYS，將其賦與空值。

* 編譯 Linux Kernel 報錯：BTF: .tmp_vmlinux.btf: pahole (pahole) is not available

> 缺乏 Debug 所需套件 Dwarves，sudo apt-get install dwarves 解決。

* 編譯 Linux Kernel 報錯：zstd: command not found

> 缺乏功能套件 zstd，sudo apt-get install zstd 解決。

* GCC 編譯 Multi-Thread Code 錯誤：undefined reference to ‘pthread_create‘

> pthread 並不屬於 Linux Standard Library，需於 gcc 編譯時加上 -lpthread 參數。

---

## 參考資料（部分參考資料已附於前述分析中）

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
