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
        scanf("%d", &block_input);

        printf("pid = %d\n", (int)getpid());
        printf("segment\tvalue\tvir_addr\tphy_addr\n");
        printf("stack\t%d\t%lx\t%lx\n", stack_value, virtual_address[0], physical_address[0]);
        printf("getpid\t%d\t%lx\t%lx\n", (int)getpid(), virtual_address[1], physical_address[1]);
        printf("heap\t%d\t%lx\t%lx\n", *(int*)heap, virtual_address[2], physical_address[2]);
        printf("bss\t%d\t%lx\t%lx\n", bss_value, virtual_address[3], physical_address[3]);
        printf("data\t%d\t%lx\t%lx\n", data_value, virtual_address[4], physical_address[4]);
        printf("code\t%d\t%lx\t%lx\n", code_function(), virtual_address[5], physical_address[5]);
    }

    return 0;
}
