#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
// #include "./Includes/clientFD.h" //For Client Fd to Communicate with Bank
#include <GateServer.h>




struct ClientParams{
    char client_hostname[MAXLINE], client_port[MAXLINE];
    int * connfd;
}typedef ClientParams;

struct tm * TIMENOW(){
  time_t rawtime;
  struct tm * timeinfo;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  
  return timeinfo;
}

void *handle_clients(void *arg){
    ClientParams * CP = arg;
    char CitiSerBUF[MAXLINE*3];
    int client_connfd= *((int*)CP->connfd);
    //free(CP->connfd);
    PaymentTokenReceiver(&client_connfd);
    struct tm * timeinfo= TIMENOW();
    sprintf(CitiSerBUF,"End Communication with Client to (%s, %s, at %s)\n",CP->client_hostname, CP->client_port,asctime(timeinfo));
    write(CItiBankLogFD,CitiSerBUF,strlen(CitiSerBUF));
    printf("End Communication with PaymentClient\n");
    close(client_connfd);
}

 void inthandler(){
     printf("\nSignal TRAPPED !!!\n");
 } 

int main(int argc, char **argv)
{
        struct sigaction newhandler; 
        sigset_t blocked; 
        newhandler.sa_handler = inthandler; 
        sigfillset(&blocked); 
        newhandler.sa_mask = blocked; 
        int i;
        for (i=1; i<31;i++)
            if (i!=9 && i!=17 && i!=19 && i!=2) /* catch all except these signals */
                if ( sigaction(i, &newhandler, NULL) == -1 )
                    printf("error with signal %d\n", i);

    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; /* Enough room for any addr */
    char client_hostname[MAXLINE], client_port[MAXLINE],Buffer[MAXLINE*3];
    listenfd = open_listenfd(argv[1]);
    CItiBankLogFD = open(".././LogFiles/GatewayServerLogs.txt",O_RDWR | O_CREAT | O_APPEND);
    while (1) {
        printf("Waiting for Clients to connect\n");
        clientlen = sizeof(struct sockaddr_storage); /* Important! */
        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
            getnameinfo((struct sockaddr *) &clientaddr, clientlen,
            client_hostname, MAXLINE, client_port, MAXLINE, 0);
             struct tm * timeinfo= TIMENOW();
            sprintf(Buffer,"Connected to (%s, %s,%s)\n", client_hostname, client_port,asctime(timeinfo));
            write(CItiBankLogFD,Buffer,strlen(Buffer));
            printf("Start Communication with Payment Client\n");
            ClientParams CP;
            strcpy(CP.client_hostname,client_hostname);
            strcpy(CP.client_port,client_port);
            CP.connfd=malloc(sizeof(int));
            CP.connfd=&connfd;
            pthread_t tid;
            int err =  pthread_create(&tid,NULL,handle_clients,(void *)&CP);
            if (err != 0)
		     	printf("cant create thread: %s \n", strerror(err));
    }
    exit(0);
}

