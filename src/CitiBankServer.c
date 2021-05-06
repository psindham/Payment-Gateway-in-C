#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include ".././Includes/header.h" // For Decoding the Details
#include <CitiBank.h>


/*Each Thread execute PAY (MAIN)*/

void* Pay(int* connfd){
    size_t n;  
    char buf[MAXLINE];
    char EndOfClient[50]="Client ENDED connection\n";
    char message[50]="Bank Payment Done with Bank-UTR-ID is \0";
    char AccNOTEXIST[50]="Account Details Doesn't Exists!!\0";
    char FailMess[MAXLINE]="Payment Failed due to Insufficient Balance\0";
        n = read(*connfd, buf, MAXLINE);
        printf("Bank server received %d bytes\n", (int)n);
    	buf[n] = '\0';
        printf("Bank server received message : %s\n", buf);
        if(strncmp(buf,"DUMMY",5)!=0){
            int Results =Transact(buf);  // [0=Account Doeesnot exists,2=Failed Due to Insuffient Balance ,other than that is UTR number]
            if(Results==0){
                write(*connfd,AccNOTEXIST,strlen(AccNOTEXIST));
            }else if( Results==2){
                write(*connfd,FailMess,strlen(FailMess));
            }else{
                sprintf(message+strlen(message),"%d",Results);
                write(*connfd,message,strlen(message));
            }
        }else{
            write(*connfd,EndOfClient,strlen(EndOfClient));
        }
}

/*Thread Handler*/

void * GatewayServer_handler(void *connfd){
    int GatewayServer_client_connfd= *((int*)connfd);
    free(connfd);
    Pay(&GatewayServer_client_connfd);
    printf("End Communication with Client\n");
    close(GatewayServer_client_connfd);
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; /* Enough room for any addr */
    char client_hostname[MAXLINE], client_port[MAXLINE];
    listenfd = open_listenfd(argv[1]);
    
    /*Initialize Mutexes*/

    pthread_mutex_init(&mymutex, NULL);
    pthread_rwlock_init(&rw_lock_Transact_money, NULL);


    while (1) {
        printf("Waiting for a new Client to connect\n");
        clientlen = sizeof(struct sockaddr_storage); /* Important! */
        
        /*Accept the Connection*/

        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        getnameinfo((struct sockaddr *) &clientaddr, clientlen,
        client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        printf("Start Communication with Client\n");
        int *sclient=malloc(sizeof(int));
        *sclient = connfd;

        /*Give that Connfd to client Ephimeral PORT on localhost*/

        pthread_t tid;
        int err =  pthread_create(&tid,NULL,GatewayServer_handler,(void *)sclient);
        if (err != 0)
             printf("cant create thread: %s\n", strerror(err));


    }

    /*uninitialize Mutexes*/
    pthread_mutex_destroy(&mymutex);
    pthread_rwlock_destroy(&rw_lock_Transact_money);
    exit(0);
}

