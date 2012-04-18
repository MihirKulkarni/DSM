#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
int ismaster;//1-master 0-slave;

void *response_function( void *ptr );
void *execution_function( void *ptr );

int main(int arg, char **argv) {
  char * masterip="127.0.0.1",*otherip="127.0.0.1";
  int mport=20000,oport=20001;
  ismaster=strcmp(argv[1],"master")==0;
  pthread_t thread1, thread2;
  int  iret1, iret2;
  int r_sock, r_connected, r_bytes_recieved , true = 1;  
  int e_sock, e_bytes_recieved;  
  struct sockaddr_in r_server_addr, r_client_addr;    
  int r_sin_size;
  struct hostent *e_host;
  struct sockaddr_in e_server_addr;  

  if(ismaster==1){
    e_host = gethostbyname(otherip);
    r_server_addr.sin_port = htons(mport);//5000
    e_server_addr.sin_port = htons(oport); //5001
  }
  else if(ismaster==0){
    e_host = gethostbyname(masterip);
    r_server_addr.sin_port = htons(oport);//5000
    e_server_addr.sin_port = htons(mport); //5001
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
  
  e_server_addr.sin_family = AF_INET;     
  e_server_addr.sin_addr = *((struct in_addr *)e_host->h_addr);
  bzero(&(e_server_addr.sin_zero),8); 
  
  if(ismaster==1){
    if (bind(r_sock, (struct sockaddr *)&r_server_addr, sizeof(struct sockaddr)) == -1) {
      perror("Unable to bind");
      exit(1);
    }
    if (listen(r_sock, 5) == -1) {
      perror("Listen");
      exit(1);
    }
    printf("\nTCPServer of response thread in master is waiting for client on port 5000\n");
    fflush(stdout);
    r_sin_size = sizeof(struct sockaddr_in);
    r_connected = accept(r_sock, (struct sockaddr *)&r_client_addr,&r_sin_size);
    printf("\nI got a connection from (%s , %d)\n\n",inet_ntoa(r_client_addr.sin_addr),ntohs(r_client_addr.sin_port));
//------------Connection between reponse of master and execution of slave established on port 5000-----------------//
//----------Now will establish connection between execution of master and response of slave on port 5001-----------//
    printf("Connection established between response of master and execution of slave on port 5000\n");
    printf("Starting execution of master to connect to response of slave on port 5001\n");
    usleep(2000000);
    if (connect(e_sock, (struct sockaddr *)&e_server_addr,sizeof(struct sockaddr)) == -1) {
      perror("Connect");
      exit(1);
    }
  }
  else if(ismaster==0){
//Connect from slave's execution thread and server's response thread
    if (connect(e_sock, (struct sockaddr *)&e_server_addr,sizeof(struct sockaddr)) == -1) {
      perror("Connect");
      exit(1);
    }

//Connection from slave's response thread to server's execution thread
    if (bind(r_sock, (struct sockaddr *)&r_server_addr, sizeof(struct sockaddr)) == -1) {
      perror("Unable to bind");
      exit(1);
    }
    if (listen(r_sock, 5) == -1) {
      perror("Listen");
      exit(1);
    }
    printf("\nTCPServer of response thread in slave is waiting for client on port 5001\n");
    fflush(stdout);
    r_sin_size = sizeof(struct sockaddr_in);
    r_connected = accept(r_sock, (struct sockaddr *)&r_client_addr,&r_sin_size);
    printf("\nI got a connection from (%s , %d)\n\n",inet_ntoa(r_client_addr.sin_addr),ntohs(r_client_addr.sin_port));
}
  /* Create independent threads each of which will execute function */
  iret1 = pthread_create( &thread1, NULL, response_function, (void*) r_connected);
  iret2 = pthread_create( &thread2, NULL, execution_function, (void*) e_sock);
  /* Wait till threads are complete before main continues. Unless we  */
  /* wait we run the risk of executing an exit which will terminate   */
  /* the process and all threads before the threads have completed.   */

  pthread_join( thread1, NULL);
  pthread_join( thread2, NULL); 

  printf("Thread 1 returns: %d\n",iret1);
  printf("Thread 2 returns: %d\n",iret2);
  close(r_sock);
  close(e_sock);
  exit(0);
}

void *response_function( void *ptr ) {
  FILE *f; 
  char *addr= mmap((void*)(1<<30), 20000*4096, PROT_READ|PROT_WRITE,  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  *(addr+(999*4096))='f';
  *(addr+(999*4096)+1)='a';

  int r_connected = (int *)ptr;
  int r_bytes_recieved , true = 1;  
  char r_send_data[4096]="response with page-999"; 
  char r_recv_data[10]; //actually recv page number - int 
  while (1) {
    r_bytes_recieved = recv(r_connected,r_recv_data,10,0);
    r_recv_data[r_bytes_recieved] = '\0';
    int request_pagenum=atoi(r_recv_data);
    if(ismaster==1)
      f=fopen("logs/master_res.log","a+");
    else 
      f=fopen("logs/slave_res.log","a+");
    fprintf(f,"\n Request for pagenumber = %d " , request_pagenum);
    fclose(f);
    fflush(stdout);
    send(r_connected, addr+(request_pagenum*4096),4096, 0); 
  }
  return 0;
}

void *execution_function( void *ptr ) {
  FILE *f;
    if(ismaster==1)
      f=fopen("logs/master_exe.log","a+");
    else 
      f=fopen("logs/slave_exe.log","a+");
  printf("execution thread\n");
  int e_sock = (int *) ptr;
  int e_bytes_recieved;  
  char e_send_data[10]; //actually send page num - int
  int request_pagenum=999;
  sprintf(e_send_data,"%d",request_pagenum);
  char e_recv_data[4096];
  send(e_sock,e_send_data,strlen(e_send_data), 0);   
  e_bytes_recieved=recv(e_sock,e_recv_data,4096,0);
  //e_recv_data[e_bytes_recieved] = '\0';
  fprintf(f,"\nRecieved data = %s %d" , e_recv_data,e_bytes_recieved);
  fclose(f);
  return 0;
}
