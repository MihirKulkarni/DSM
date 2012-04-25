/*	Project: Distributed Shared Memory
 *	EECS 218 - Distributed Computer Systems
 *	
 *	Member-1
 *	Name : Karthik Ragunath Balasundaram
 *	Student ID : 78806170
 *	UCI Student ID : kbalasun
 *
 *	Member-2
 *	Name : Mihir Kulkarni
 *	Student ID : 89481531
 *	UCI Student ID : mihirk
 *
 *	Please refer to README for the steps to run this program.
 *
 *	NOTE: Please start the "master" before staring execution of "slave"
 *				for proper results.
 *
 *	1. Initially, we divide the shared memory between the master and the
 *		slave so that each has ownership of half the memory.
 *
 *	2. Whenever a process requests a page, we transfer the ownership of
 *		the page to the requesting process. So when the current process
 *		again needs that same page, it has to request it from the other process.
 *
 *	3. Ownership of pages is achieved using the protection mode of "mprotect".
 *		If a page is "mprotect" in PROT_NONE mode it means that the process
 *		doesn't have ownership of the particular page. On the other hand if
 *		"mprotect" is in PROT_WRITE mode, it means that the process owns the page.
 *
 *	4. We have also made use of page level mutex locking to avoid various
 *		problems in a Distributed Shared Memory. It also helps us to avoid
 *		deadlocks. (pthread_mutex)
 *
 *	5. We establish the socket connections in "initializeDSM" and use pthread
 *		only to send and receive pages.
 *
 *	6. A response thread in both master and slave keeps listening on a port
 *		for page requests. The response thread itself is responsible for 
 *		sending the page requested by the other process. Again it makes use of
 *		per page locks to avoid data inconsistency.
 *
 */



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

pthread_mutex_t *global_mutex;
int pagesize = 4096;
int is_master = 0;
int e_sock,r_sock;															// Execution thread socket descriptor for master and slave respectively
char *base_addr = (char*)(1<<30);								// 1GB
void *response_function ( void *ptr );

/*	The "handler" function specifies the action to be taken
 *	in receipt of a specific SIGNAL.
 *	This is called when we have a PAGE FAULT
 */

void handler (int cause, siginfo_t *si, void *uap) {

  char *fault_addr = si->si_addr;
  int e_bytes_recieved;  
  char e_send_data[10];													// Actually send page num - int
  int request_pagenum = 0;
	sprintf(e_send_data,"%d",request_pagenum);		// e_send_data has the pagenum which is requested
  char e_recv_data[4096];												// Buffer to store incoming page

  request_pagenum = ((int)(fault_addr-base_addr)) / pagesize;
  pthread_mutex_lock(&global_mutex[request_pagenum]);			// Lock the page using per page mutex locking

	sprintf (e_send_data,"%d",request_pagenum);
  send (e_sock,e_send_data,strlen(e_send_data), 0);
  e_bytes_recieved=recv(e_sock,e_recv_data,4096,0);				// Receive page in buffer

  if (mprotect(base_addr+(request_pagenum*pagesize), pagesize, PROT_WRITE)) {		// Set protection of received page to PROT_WRITE
    perror("mprotect");
    exit(1);
  }

  memcpy(base_addr+(request_pagenum*pagesize),e_recv_data,pagesize);		// Copy the page to memory
  pthread_mutex_unlock(&global_mutex[request_pagenum]);		// Unlock the page mutex

}

void initializeDSM (int ismaster, char * masterip, int mport, char *otherip, int oport, int numpagestoalloc) {
	is_master=ismaster;
	global_mutex = malloc(sizeof(pthread_mutex_t)*numpagestoalloc);			// "malloc" mutex; 1 for each page

  char *addr= mmap((void*)(1<<30), numpagestoalloc*4096, PROT_READ|PROT_WRITE,  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  struct sigaction sa;
  pagesize = sysconf(_SC_PAGESIZE);

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
	
	if (ismaster==1) {
    char *start_addr = addr + pagesize * (numpagestoalloc/2);
    int numpagestoprotect = numpagestoalloc - (numpagestoalloc/2);
    char *own_start_addr = addr;
    int own_numpagestoprotect = (numpagestoalloc/2);
		if (mprotect(start_addr, pagesize * numpagestoprotect, PROT_NONE)) {
      perror("mprotect");
      exit(1);
    }
  }

	if (ismaster==0) {
    char *start_addr = addr;
    int numpagestoprotect = (numpagestoalloc/2);
    char *own_start_addr = addr + pagesize * (numpagestoalloc/2);
    int own_numpagestoprotect = numpagestoalloc - (numpagestoalloc/2);
    if (mprotect(start_addr, pagesize * numpagestoprotect, PROT_NONE)) {
      perror("mprotect");
      exit(1);
    }
}

/* Configuring TCP sockets */

	pthread_t thread1, thread2;
  int  iret1, iret2;
  int r_connected, r_bytes_recieved, true=1;
  int e_bytes_recieved;  //e_sock declared globally
  struct sockaddr_in r_server_addr, r_client_addr;    
  int r_sin_size;
  struct hostent *e_host;
  struct sockaddr_in e_server_addr;  

  if (ismaster == 1) {
    e_host = gethostbyname(otherip);
    r_server_addr.sin_port = htons(mport);//20000
    e_server_addr.sin_port = htons(oport); //20001
  }
  else if (ismaster == 0) {
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

  if(ismaster == 1){
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
  else if (ismaster==0) {
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
}

void *response_function (void *ptr) {

  int r_connected = (int *)ptr;
  int r_bytes_recieved , true = 1;  
  char r_send_data[4096]; 
  char r_recv_data[10]; //actually recv page number - int 

	while (1) {

    r_bytes_recieved = recv(r_connected,r_recv_data,10,0);
    r_recv_data[r_bytes_recieved] = '\0';
    int request_pagenum=atoi(r_recv_data);
    pthread_mutex_lock(&global_mutex[request_pagenum]);

    fflush(stdout);

    if (mprotect(base_addr+(request_pagenum*pagesize), pagesize, PROT_READ)) {
      perror("mprotect");
      exit(1);
    }

    send(r_connected, base_addr+(request_pagenum*pagesize),pagesize, 0); 

    if (mprotect(base_addr+(request_pagenum*pagesize), pagesize, PROT_NONE)) {
      perror("mprotect");
      exit(1);
    }

    pthread_mutex_unlock(&global_mutex[request_pagenum]);

  }

	return 0;

}
void* getsharedregion() {

	return (void*)(1<<30);

}

void TerminateDSM() {

	close(e_sock);
	close(r_sock);

}
