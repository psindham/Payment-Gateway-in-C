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
#include "./Includes/GateServer.h" // For 
#include "./Includes/clientFD.h" //For Client Fd to Communicate with Bank

/* Misc constants */
#define LISTENQ  1024  /* Second argument to listen() */
#define TRANSACTIONRECORD 29
#define ONLYTRANPASS 19
#define PAID 20

int CItiBankLogFD;
pthread_rwlock_t  rw_lock_payment;
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

int open_listenfd(char *port) 
{
    struct addrinfo hints, *listp, *p;
    int listenfd, optval=1;
	char host[MAXLINE],service[MAXLINE];
    int flags;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             /* Accept connections */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* ... on any IP address AI_PASSIVE - used on server for TCP passive connection, AI_ADDRCONFIG - to use both IPv4 and IPv6 addresses */
    hints.ai_flags |= AI_NUMERICSERV;            /* ... using port number instead of service name*/
    getaddrinfo("127.0.0.1", port, &hints, &listp);

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue;  /* Socket failed, try the next */

        /* Eliminates "Address already in use" error from bind */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,    //line:netp:csapp:setsockopt
                   (const void *)&optval , sizeof(int));

		flags = NI_NUMERICHOST | NI_NUMERICSERV; /* Display address string instead of domain name and port number instead of service name */
		getnameinfo(p->ai_addr, p->ai_addrlen, host, MAXLINE, service, MAXLINE, flags);
        printf("host:%s, service:%s\n", host, service);

        /* Bind the descriptor to the address */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; /* Success */
        close(listenfd); /* Bind failed, try the next */
    }

    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* No address worked */
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
	    return -1;
    }
    return listenfd;
}

char* TransactPayment(char buf[])
{
    int clientfd;
    char *host="localhost", *port="15001";    //Bank server is running on the PORT no. 15001 
    clientfd = open_clientfd(host, port);     //create a client FD to communicate with bank 
    write(clientfd, buf, strlen(buf));
    read(clientfd, buf, MAXLINE);
    close(clientfd);
    return (char *)buf;
}

int UpdateGatewayTrasaction(char buf[])
{
   char c[MAXLINE];
   size_t len = 0;
   int srcfd = open("DataFiles/PaymentTokensData.txt",O_RDWR);
   int read_res;
    while(1)
	{
        pthread_rwlock_rdlock(&rw_lock_payment);
        read_res=read(srcfd,c,TRANSACTIONRECORD);
        pthread_rwlock_unlock(&rw_lock_payment);
        if(read_res==0)
            break;
            if(strncmp(c,buf,ONLYTRANPASS)==0){   
                if(c[PAID]=='0')
                {
                    char temp[2]="1";
                    pthread_rwlock_wrlock(&rw_lock_payment);
                        if(lseek(srcfd,-9,SEEK_CUR)==-1)
                            printf("Error-11");
                        if(write(srcfd,temp,1)==-1)
                            printf("Error-12");
                    pthread_rwlock_unlock(&rw_lock_payment);
                    close(srcfd);
                    return 1; //payment update done!
                }
            }
    }
   close(srcfd);
   return 0; //Payment Doesnot exists
}

int PaymentTokenValidator(char buf[])
{
   char c[MAXLINE];
   size_t len = 0;
   int srcfd = open("DataFiles/PaymentTokensData.txt",O_RDONLY);
    while (read(srcfd,c,TRANSACTIONRECORD))
	{
           // printf("%s",c);     
            if(strncmp(c,buf,ONLYTRANPASS)==0)
            {   
                if(c[PAID]!='0')
                {
                    close(srcfd);
                    return 2; //payment already done!
                }else{
                    char  amount[6];
                    strncpy(amount,c+22,5);
                    int amt = atoi(amount);
                    close(srcfd);
                    return amt;
                }
            }
    }
   close(srcfd);
   return 0; //Payment Doesnot exists
}


void* PaymentTokenReceiver(int* connfd){
    size_t n;
    char buf[MAXLINE];
    char temp[MAXLINE];
    char *SuccessMess="Success";
    char FailMess[MAXLINE]="Payment Failed\0";
    char PaidMess[25]="Payment Already paid\0";
    char BankReply[50];
    int flag=1;
    while(1){
        memset(buf,0,sizeof buf);
        n = read(*connfd, buf, MAXLINE); //Read TransactionID , passcode
        if(buf[0]=='\0')
            break; // End the client communication 
         char temp2[MAXLINE];
         strcpy(temp2,buf);
        if(buf[0]=='0')
            break;
        printf("Gateway server received %d bytes\n", (int)n);
	    buf[n] = '\0';
         //    printf("Gateway server received message : %s\n", buf);
             int resultPaymentValid=PaymentTokenValidator(buf); // validate Payment Token [0=Token Doesn't Exists,2=Already Paid,other than that  is Amount]
             char temp[10];
             char merchant[5];
             strncpy(merchant,buf,4); 
             sprintf(temp,"%d",resultPaymentValid);
            //  printf("%d",resultPaymentValid);
            if(resultPaymentValid!=0 && resultPaymentValid!=2){ // returns the amount

                write(*connfd,temp,strlen(temp));  //valid for Payment 
                memset(buf, 0, sizeof buf);
                n = read(*connfd, buf, MAXLINE); //then read userId, password
                printf("Gateway server received %d bytes\n", (int)n);

                char Price[10];
                sprintf(Price,"%d",resultPaymentValid);
                strcat(buf," ");
                strcat(buf,merchant);
                strcat(buf," ");
                strcat(buf,Price);
                printf("Gateway server received message : %s\n", buf);

             if(strncmp(buf,"DUMMY",5)!=0){

                strcpy(BankReply,TransactPayment(buf));// Reply From the Bank

                if(strncmp(BankReply,"Account",7)!=0 && strncmp(BankReply,"Payment",7)!=0)
                {
                    if(UpdateGatewayTrasaction(temp2)==1)
                    {
                        printf("Gateway Transaction Payment Table updated \n");
                        pthread_t threadID;
                        params  p;
                        strcpy(p.UTR,BankReply);
                        strcpy(p.Details,temp2);
                        p.payment =resultPaymentValid;

                        int err = pthread_create(&threadID,NULL,threadSaveTransaction,&p);
                           // SaveTransaction(BankReply,temp2,resultPaymentValid);// want to do thread 
                       	pthread_join(threadID, NULL);
                        write(*connfd,BankReply,strlen(BankReply));
                        memset(BankReply,0,sizeof BankReply);
                    }else{
                        printf("Gateway Transaction Payment Table Not updated \n");
                        write(*connfd,BankReply,strlen(BankReply));
                        memset(BankReply,0,sizeof BankReply);
                    }
                }else{
                     write(*connfd,BankReply,strlen(BankReply));
                     memset(BankReply,0,sizeof BankReply);
                }
             }else{
                  memset(buf, 0, sizeof buf);
                  write(*connfd,FailMess,strlen(FailMess));//Failure of Payment
             }
            }else if(resultPaymentValid==2){ //if payment is already done
                memset(buf, 0, sizeof buf);
                write(*connfd,PaidMess,strlen(PaidMess));//Payment already done
            }else if(resultPaymentValid==0){
                memset(buf, 0, sizeof buf);
                write(*connfd,FailMess,strlen(FailMess));//Failure of Payment
            }
    }
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
    CItiBankLogFD = open("LogFiles/GatewayServerLogs.txt",O_RDWR | O_CREAT | O_APPEND);
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

