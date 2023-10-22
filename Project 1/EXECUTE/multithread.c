#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>      /* Definition of SYS_* constants */
#include <unistd.h>

extern void *func1(void *);
extern void *func2(void *);
extern int main();
void * my_get_physical_addresses(void *);

struct data_
{
int  id ;
char name[16] ;
} ;
typedef struct data_ sdata ;
static __thread  sdata  tx ;  //thread local variable

int a=123;  //global variable

void hello(int pid)
{
int b=10;   //local varialbe

b=b+pid;
			 //global variable
printf("In thread %d \nthe value of gloable varialbe a is %d, the offset of the logical address of  a is %p, ", pid, a, &a);
printf("the physical address of global variable a is %p\n", my_get_physical_addresses(&a) );
			 //local variable
printf("the value of local varialbe b is %d, the offset of the logical address of b is %p, ", b, &b);
printf("the physical address of local variable b is %p\n", my_get_physical_addresses(&b));
			 //thread local variable
printf("the offset of the logical address of thread local varialbe tx is %p, ", &tx);
printf("the physical address of thread local variable tx is %p\n", my_get_physical_addresses(&tx));
			 //function
printf("the offset of the logical address of function hello is %p, ", hello);
printf("the physical address of function hello is %p\n", my_get_physical_addresses(hello));
printf("the offset of the logical address of function func1 is %p, ", func1);
printf("the physical address of function func1 is %p\n", my_get_physical_addresses(func1));
printf("the offset of the logical address of function func2 is %p, ", func2);
printf("the physical address of function func2 is %p\n", my_get_physical_addresses(func2));
printf("the offset of the logical address of function main is %p, ", main);
printf("the physical address of function main is %p\n", my_get_physical_addresses(main));
			 //library function
printf("the offset of the logical address of library function printf is %p, ", printf);
printf("the physical address of library function printf is %p\n", my_get_physical_addresses(printf));
printf("====================================================================================================================\n");
}  

void *func1(void *arg)
{
char *p = (char*) arg ;
int pid  ;
pid =  syscall( __NR_gettid );
tx.id = pid ;
strcpy(tx.name,p) ;
printf("I am thread with ID %d executing func1().\n",pid);
hello(pid);
while(1)
{
  //printf("(%d)(%s)\n",tx.id,tx.name) ;
  sleep(1) ;
}
}

void *func2(void *arg)
{
char *p = (char*) arg ;
int pid  ;
pid =  syscall( __NR_gettid );
tx.id = pid ;
strcpy(tx.name,p) ;
printf("I am thread with ID %d executing func2().\n",pid);
hello(pid);
while(1)
{
  //printf("(%d)(%s)\n",tx.id,tx.name) ;
  sleep(2) ;
}
}
int main()
{ 
pthread_t id[2];
char p[2][16] ;
strcpy(p[0],"Thread1") ;
pthread_create(&id[0],NULL,func1,(void *)p[0]);
strcpy(p[1],"Thread2") ;
pthread_create(&id[1],NULL,func2,(void *)p[1] );

int pid  ;
pid =  syscall( __NR_gettid );
tx.id = pid ;
strcpy(tx.name,"MAIN") ;
printf("I am main thread with ID %d.\n", pid);
hello(pid);
while(1)
{
 //printf("(%d)(%s)\n",tx.id,tx.name) ;
 sleep(5) ;
}

}