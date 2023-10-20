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
