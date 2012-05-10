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


void *response_function( void *ptr );
void *execution_function( void *ptr );

main() {
  pthread_t thread1, thread2;
  char *message1 = "Thread 1";
  char *message2 = "Thread 2";
  int  iret1, iret2;


  int e_sock, e_bytes_recieved;  
  char e_send_data[1024], e_recv_data[1024];
  struct hostent *e_host;
  struct sockaddr_in e_server_addr;  

  e_host = gethostbyname("127.0.0.1");

  if ((e_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket");
    exit(1);
  }

  e_server_addr.sin_family = AF_INET;     
  e_server_addr.sin_port = htons(5000);
  e_server_addr.sin_addr = *((struct in_addr *)e_host->h_addr);
  bzero(&(e_server_addr.sin_zero),8); 

  if (connect(e_sock, (struct sockaddr *)&e_server_addr,
             sizeof(struct sockaddr)) == -1) {
    perror("Connect");
    exit(1);
  }

printf("Connection established between execution of slave and response of master on port 5000\n");
printf("Starting response of slave to listen for execution of master on port 5001\n");

  int r_sock, r_connected, r_bytes_recieved , true = 1;  
  char r_send_data [1024] , r_recv_data[1024];       

  struct sockaddr_in r_server_addr, r_client_addr;    
  int r_sin_size;
      
  if ((r_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
    exit(1);
  }

  if (setsockopt(r_sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) == -1) {
		perror("Setsockopt");
		exit(1);
  }
        
  r_server_addr.sin_family = AF_INET;
  r_server_addr.sin_port = htons(5001);
  r_server_addr.sin_addr.s_addr = INADDR_ANY; 
  bzero(&(r_server_addr.sin_zero),8); 

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

  printf("\nI got a connection from (%s , %d)\n\n",
  inet_ntoa(r_client_addr.sin_addr),ntohs(r_client_addr.sin_port));

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
    int r_connected = (int *)ptr;
    int r_bytes_recieved , true = 1;  
    char r_send_data [1024] , r_recv_data[1024];       
    while (1) {
      r_bytes_recieved = recv(r_connected,r_recv_data,1024,0);

      r_recv_data[r_bytes_recieved] = '\0';

      if (strcmp(r_recv_data , "q") == 0 || strcmp(r_recv_data , "Q") == 0) {
	printf("Closing connection with execution thread of slave..\n");
        close(r_connected);
        break;
      }

      else 
        printf("\n RECIEVED DATA = %s " , r_recv_data);
      fflush(stdout);

      printf("\n SEND (q or Q to quit) : ");
      gets(r_send_data);
              
      if (strcmp(r_send_data , "q") == 0 || strcmp(r_send_data , "Q") == 0) {
        send(r_connected, r_send_data,strlen(r_send_data), 0); 
        close(r_connected);
        break;
      }
               
      else
        send(r_connected, r_send_data,strlen(r_send_data), 0);  
    }
//  close(sock);
  return 0;

}

void *execution_function( void *ptr ) {
	int e_sock = (int *) ptr;
        int e_bytes_recieved;  
        char e_send_data[1024],e_recv_data[1024];
        while(1)
        {
           printf("\nSEND (q or Q to quit) : ");
           gets(e_send_data);
           
          if (strcmp(e_send_data , "q") != 0 && strcmp(e_send_data , "Q") != 0)
           send(e_sock,e_send_data,strlen(e_send_data), 0); 

          else
          {
           send(e_sock,e_send_data,strlen(e_send_data), 0);   
           close(e_sock);
           break;
          }
        
          e_bytes_recieved=recv(e_sock,e_recv_data,1024,0);
          e_recv_data[e_bytes_recieved] = '\0';
 
          if (strcmp(e_recv_data , "q") == 0 || strcmp(e_recv_data , "Q") == 0)
          {
           close(e_sock);
           break;
          }

          else
           printf("\nRecieved data = %s " , e_recv_data);
        }   
return 0;
}
