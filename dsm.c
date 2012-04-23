#include <time.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
pthread_mutex_t global_mutex=PTHREAD_MUTEX_INITIALIZER;
int pagesize=4096;
int is_master=0;
int e_sock,r_sock; // execution thread socket descriptor
char *base_addr=(char*)(1<<30);
void *response_function( void *ptr );
char timestamp[25];

void ret_timestamp(char* buffer){
time_t timer;
struct tm* tm_info;
time(&timer);
tm_info = localtime(&timer);
strftime(buffer, 25, "%m/%d/%Y %H:%M:%S", tm_info);
}
void handler(int cause, siginfo_t *si, void *uap) {
//printf("sigfault");
  pthread_mutex_lock(&global_mutex);
//printf("locked\n");

  char *fault_addr = si->si_addr;
  int e_bytes_recieved;  
  char e_send_data[10]; //actually send page num - int
  int request_pagenum=0;
  sprintf(e_send_data,"%d",request_pagenum);
  char e_recv_data[4096];
/*  FILE *f;
  if(is_master==1)
      f=fopen("master_exe.log","a+");
    else 
      f=fopen("slave_exe.log","a+");*/
  ret_timestamp(timestamp);
  //fprintf(f, "\n%s : SIGSEGV raised pagefault at address %p %p %d\n",timestamp, si->si_addr,base_addr, (int)(fault_addr-base_addr));
  request_pagenum=((int)(fault_addr-base_addr))/pagesize;


//char y[10];
sprintf(e_send_data,"%d",request_pagenum);


//printf("%d\n",request_pagenum);
 // fprintf(f, "Request for Page - %d from other process\n",request_pagenum);
  send(e_sock,e_send_data,strlen(e_send_data), 0);   
  e_bytes_recieved=recv(e_sock,e_recv_data,4096,0);
 // e_recv_data[e_bytes_recieved] = '\0';
  int *x;
if(request_pagenum==7)
x=(int *)(e_recv_data+3328);
else
x=(int *)(e_recv_data);

//  fprintf(f,"\n%s : Recieved data = %d %d" ,timestamp,*x,e_bytes_recieved);
//  fclose(f);
  if (mprotect(base_addr+(request_pagenum*pagesize), pagesize,PROT_WRITE)) {
    perror("mprotect");
    exit(1);
  }
  memcpy(base_addr+(request_pagenum*pagesize),e_recv_data,pagesize);
  pthread_mutex_unlock(&global_mutex);
//printf("unlocked\n");

}



void initializeDSM(int ismaster, char * masterip, int mport, char *otherip, int oport, int numpagestoalloc){
  is_master=ismaster;//remove finally
  char *addr= mmap((void*)(1<<30), numpagestoalloc*4096, PROT_READ|PROT_WRITE,  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  struct sigaction sa;
  pagesize = sysconf(_SC_PAGESIZE);
//  printf("--- Initialising Distributed Shared Memory for %d pages--- at Base Address %d\n",numpagestoalloc,(int*)addr);

/* Configuring Signal Handle */
  sa.sa_sigaction = handler;
  sigemptyset( &sa.sa_mask );
  sa.sa_flags = SA_SIGINFO;
  if (sigaction (SIGSEGV, &sa, 0)) {
    perror("sigaction");
    exit(1);
  }
signal(SIGPIPE,SIG_IGN);
/*  Taking ownership of memory */
  if(ismaster==1){
    char *start_addr=addr+pagesize*(numpagestoalloc/2);
    int numpagestoprotect=numpagestoalloc-(numpagestoalloc/2);
    char *own_start_addr=addr;
    int own_numpagestoprotect=(numpagestoalloc/2);
  // printf("\nTaking ownership of %d pages from %p to %p\n",own_numpagestoprotect,own_start_addr,own_start_addr+pagesize*own_numpagestoprotect-1);
    if (mprotect(start_addr, pagesize*numpagestoprotect, PROT_NONE)) {
      perror("mprotect");
      exit(1);
    }
  }
  if(ismaster==0){
    char *start_addr=addr;
    int numpagestoprotect=(numpagestoalloc/2);
    char *own_start_addr=addr+pagesize*(numpagestoalloc/2);
    int own_numpagestoprotect=numpagestoalloc-(numpagestoalloc/2);
//    printf("\nTaking ownership of %d pages from %p to %p\n",own_numpagestoprotect,own_start_addr,own_start_addr+pagesize*own_numpagestoprotect-1);
    if (mprotect(start_addr, pagesize*numpagestoprotect, PROT_NONE)) {
      perror("mprotect");
      exit(1);
    }
}
/* Configuring TCP sockets */
  pthread_t thread1, thread2;
  int  iret1, iret2;
  int r_connected, r_bytes_recieved,true=1;  
  int e_bytes_recieved;  //e_sock declared globally
  struct sockaddr_in r_server_addr, r_client_addr;    
  int r_sin_size;
  struct hostent *e_host;
  struct sockaddr_in e_server_addr;  

  if(ismaster==1){
    e_host = gethostbyname(otherip);
    r_server_addr.sin_port = htons(mport);//20000
    e_server_addr.sin_port = htons(oport); //20001
  }
  else if(ismaster==0){
    e_host = gethostbyname(masterip);
    r_server_addr.sin_port = htons(oport);//20001
    e_server_addr.sin_port = htons(mport); //20000
  }
  if ((r_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket");
    exit(1);
  }
  if (setsockopt(r_sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) == -1) {
    perror("Setsockopt");
    exit(1);
  }

  r_server_addr.sin_family = AF_INET;
  r_server_addr.sin_addr.s_addr = INADDR_ANY; 
  bzero(&(r_server_addr.sin_zero),8); 
  if ((e_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket");
    exit(1);
  }
if (setsockopt(e_sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) == -1) {
    perror("Setsockopt");
    exit(1);
  }

  e_server_addr.sin_family = AF_INET;     
  e_server_addr.sin_addr = *((struct in_addr *)e_host->h_addr);
  bzero(&(e_server_addr.sin_zero),8); 

/* Establishing connections */

  if(ismaster==1){
//Connect from masters's execution thread and slaves's response thread
    if (bind(r_sock, (struct sockaddr *)&r_server_addr, sizeof(struct sockaddr)) == -1) {
      perror("Unable to bind");
      exit(1);
    }

    if (listen(r_sock, 5) == -1) {
      perror("Listen");
      exit(1);
    }
    fflush(stdout);
    r_sin_size = sizeof(struct sockaddr_in);
    r_connected = accept(r_sock, (struct sockaddr *)&r_client_addr,&r_sin_size);
    usleep(2000000); //waiting for delay in 1st TCP socket establishment
//Connect from masters's response thread and slaves's execution thread
    if (connect(e_sock, (struct sockaddr *)&e_server_addr,sizeof(struct sockaddr)) == -1) {
      perror("Connect");
      exit(1);
    }
  }
  else if(ismaster==0){
//Connect from slave's execution thread and master's response thread
    if (connect(e_sock, (struct sockaddr *)&e_server_addr,sizeof(struct sockaddr)) == -1) {
      perror("Connect");
      exit(1);
    }
//Connection from slave's response thread to master's execution thread
    if (bind(r_sock, (struct sockaddr *)&r_server_addr, sizeof(struct sockaddr)) == -1) {
      perror("Unable to bind");
      exit(1);
    }
    if (listen(r_sock, 5) == -1) {
      perror("Listen");
      exit(1);
    }
    fflush(stdout);
    r_sin_size = sizeof(struct sockaddr_in);
    r_connected = accept(r_sock, (struct sockaddr *)&r_client_addr,&r_sin_size);
}
  /* Create independent threads each of which will execute function */
  iret1 = pthread_create( &thread1, NULL, response_function, (void*) r_connected);
//  iret2 = pthread_create( &thread2, NULL, execution_function, (void*) e_sock);
  /* Wait till threads are complete before main continues. Unless we  */
  /* wait we run the risk of executing an exit which will terminate   */
  /* the process and all threads before the threads have completed.   */

//  pthread_join( thread1, NULL);
//  pthread_join( thread2, NULL); 


/*test
    printf( "About to read 'a'\n");
    printf( "address %p contains '%c'\n", tempadd, *tempadd);
*/
//  printf("Thread 1 returns: %d\n",iret1);
//  printf("Thread 2 returns: %d\n",iret2);
//  close(r_sock);
//  close(e_sock);
//  exit(0);
  
}

void *response_function( void *ptr ) {
  //FILE *f; 
  int r_connected = (int *)ptr;
  int r_bytes_recieved , true = 1;  
  char r_send_data[4096]; 
  char r_recv_data[10]; //actually recv page number - int 
  while (1) {
    r_bytes_recieved = recv(r_connected,r_recv_data,10,0);
    pthread_mutex_lock(&global_mutex);
//    printf("locked\n");
    r_recv_data[r_bytes_recieved] = '\0';
//printf("\nresponse:%s",r_recv_data);
    int request_pagenum=atoi(r_recv_data);
 /*   if(is_master==1)
      f=fopen("master_res.log","a+");
    else 
      f=fopen("slave_res.log","a+");*/
    ret_timestamp(timestamp);
//    fprintf(f,"\n %s : Request for pagenumber = %d " ,timestamp, request_pagenum);

    fflush(stdout);
    if (mprotect(base_addr+(request_pagenum*pagesize), pagesize, PROT_READ)) {
      perror("mprotect");
      exit(1);
    }
    send(r_connected, base_addr+(request_pagenum*pagesize),pagesize, 0); 
int *x;
if(request_pagenum==7)
x=(int*)(base_addr+(request_pagenum*pagesize)+3328);
else
x=(int*)(base_addr+(request_pagenum*pagesize));

  //  fprintf(f,"\n : Sending from  = %p data: %d " ,base_addr+(request_pagenum*pagesize),*x);

    if (mprotect(base_addr+(request_pagenum*pagesize), pagesize, PROT_NONE)) {
      perror("mprotect");
      exit(1);
    }
 //   fclose(f);
    pthread_mutex_unlock(&global_mutex);
//usleep(500000);
//		printf("unlocked\n");
  }
return 0;
}
void* getsharedregion(){
//printf("--- Getting Shared Region --- at Base Address %d\n",1<<30);
return (void*)(1<<30);
}

void TerminateDSM(){
close(e_sock);
close(r_sock);
}

