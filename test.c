#include<stdlib.h>
#include<stdio.h>

#include "dsm.h"


struct testlist {
  void *mypt;
  struct testlist *next;
};

#define LOCK_PREFIX \
  ".section .smp_locks,\"a\"\n"   \
  "  .align 4\n"                  \
  "  .long 661f\n"             /* address */\
  ".previous\n"                   \
  "661:\n\tlock; "


static inline void atomic_inc(volatile int *v) {
  __asm__ __volatile__ (LOCK_PREFIX "incl %0"
                        : "+m" (*v));
}


int main(int arg, char **argv) {
  int i;
  int master=strcmp(argv[1],"master")==0;
  char *masterip=argv[2];
  char *otherip=argv[3];
  int testnumber=atoi(argv[4]);

  printf("master=%d masterip=%s otherip=%s\n",master, masterip, otherip);
  initializeDSM(master, masterip, 40000, otherip, 40001, 10000);

  void *region = getsharedregion();

switch(testnumber) {
  case 0:
    //simple counting test
    if (master) {
      int * p=(int *) region;
      int * p2=((int *) region)+1;
      int i=0;
      for(;1;i++) {
	*p=i;
	usleep(500000);
	printf("%ld\n",*p2);
      }
    } else {
      while(1) {
	int *p=(int *)region;
	int * p2=((int *) region)+1;
	usleep(500000);
	printf("%ld\n",*p);
	*p2+=5;
      }
    }
    break;
  case 1:
    //make sure pages conflicts work correctly
    if (master) {
      int * p=(int *) region;
      int * p2=((int *) region)+1;
      int i=0;
      int num=9000;
      for(;i<num;i++) {
	(*p)++;
	usleep(2);
      }
      printf("p=%ld num=%ld should be equal\n", *p, num);
      sleep(10);//give the other one time to finish
    } else {
      int *p=(int *)region;
      int * p2=((int *) region)+1;
      int i=0;
      int num=8000;
      for(;i<num;i++) {
	(*p2)++;
	usleep(2);
      }
      printf("p2=%ld num=%ld should be equal\n", *p2, num);
      sleep(10);//give the other one time to finish
    }
    break;
  case 2:
    //chase linked lists
    if (master) {
      struct testlist * ptr=(struct testlist *)region;
      int i=0;
      for(i=0;i<10000;i++) {
	struct testlist *ptrnew=ptr+1;
	ptr->mypt=ptr;
	ptr->next=ptrnew;
	ptr=ptrnew;
	usleep(2);
      }
      sleep(10);//give the other one time to finish
    } else {
      struct testlist *ptr=(struct testlist *)region;
      int i=0;
      for(i=0;i<10000;i++) {
	while(ptr->next==NULL)
	  ;//wait for the pointer to be set
	if (ptr!=ptr->mypt)
	  printf("mismatch error:  ptr=%lx myptr=%lx should match\n", ptr, ptr->mypt);
	ptr=ptr->next;
      }
      sleep(10);//give the other one time to finish
    }
    break;
  case 3:
    //ordering test
    if (master) {
      volatile int * ptr=(int *)region;
      volatile int * ptr2=(int *)region+8000;
      int i;
      sleep(1);//give the other thread time to start
      for(i=0;i<=10000;i++) {
	(*ptr)=i;
	(*ptr2)=i;
	usleep(1);
      }
      sleep(10);//give the other one time to finish
    } else {
      volatile int * ptr=(int *)region;
      volatile int * ptr2=(int *)region+8000;
      while(1) {
	int v1=*ptr2;
	int v2=*ptr;
	if (v2<v1) {
	  printf("ERROR in ordering of reads/writes\n");
	}
	if ((v2%1000)==0)//print something every once in a while to see that it is running
	  printf("%d %d\n",v1,v2);

	if (v2==10000)
	  break;
      }
    }
    break;
  case 4:
    //mutual counting test -- short
    if (master) {
      volatile int * p=(volatile int *) region;
      int i=0;
      for(;i<20000;i++) {
	atomic_inc(p);
	usleep(1);
      }
      printf("%d -one of the two machines should match 40000\n",*p);
      sleep(10);//let the other thread finish
    } else {
      volatile int *p=(volatile int *)region;
      int i=0;
      for(;i<20000;i++) {
	atomic_inc(p);
	usleep(1);
      }
      printf("%d -one of the two machines should match 40000\n",*p);
      sleep(10);//let the other thread finish
    }
    break;
  case 5:
    //mutual counting test -- long
    if (master) {
      volatile int * p=(volatile int *) region;
      int i=0;
      for(;i<200000;i++) {
	atomic_inc(p);
	usleep(1);
      }
      printf("%d -one of the two machines should match 400000\n",*p);
      sleep(10);//let the other thread finish
    } else {
      volatile int *p=(volatile int *)region;
      int i=0;
      for(;i<200000;i++) {
	atomic_inc(p);
	usleep(1);
      }
      printf("%d -one of the two machines should match 400000\n",*p);
      sleep(10);//let the other thread finish
    }
    break;
  }
}
