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
//include <netinet/in.h>
//include <arpa/inet.h>

/* Misc constants */
#define	MAXLINE	 8192  /* Max text line length */
#define LISTENQ  1024  /* Second argument to listen()*/

pthread_rwlock_t rw_lock_deduct_money;
pthread_mutex_t mymutex;


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
    getaddrinfo(NULL, port, &hints, &listp);

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

int ReturnUTRId(){
 int TransFD = open("DataFiles/TransactionID.txt",O_RDWR);
    unsigned int ID ;
    char bufferOFDATA[10];
    memset(bufferOFDATA, 0, sizeof bufferOFDATA);
    pthread_mutex_lock(&mymutex);
        read(TransFD,bufferOFDATA,8);
        ID=atoi(bufferOFDATA);
        int previousID =ID;
        int NextID =++ID;
        if(lseek(TransFD,-8,SEEK_CUR)==-1)
        {
            printf("Error-SEEK in File TransactionID.txt\n");
            return 0;
        }
        sprintf(bufferOFDATA,"%d",NextID);
        if(write(TransFD,bufferOFDATA,8)==-1)
        {
            printf("Error-WRITE in File TransactionID.txt\n");
            return 0;
        }   
        close(TransFD);
    pthread_mutex_unlock(&mymutex);
    return previousID;
}
int DeductMoney(int *srcfd,int RemainingAmount)
{
    int n= RemainingAmount;
    int count = 0;
    while (n != 0) {
        n /= 10;     // n = n/10
        ++count;
    }
    char buf[11];
    int i;
    for(i=0;i<10-count;i++)
    {
        buf[i]='0';
    }
    sprintf(buf+i,"%d",RemainingAmount);
    buf[11]='\0';
    
    if(lseek(*srcfd,-12,SEEK_CUR)==-1)
    {
        printf("Error-SEEK in File NetBankingUsers.txt\n");
        return 0;
    }
    pthread_rwlock_wrlock(&rw_lock_deduct_money);
            if(write(*srcfd,buf,10)==-1)
            {
                printf("Error-WRITE in File NetBankingUsers.txt\n");
                return 0;
            }
    pthread_rwlock_unlock(&rw_lock_deduct_money);
     return 1;
}
int Transact(char data[])
{
    char c[LISTENQ];
    size_t len = 0;
    int srcfd = open("DataFiles/NetBankingUsers.txt", O_RDWR);
  //  pthread_rwlock_rdlock(&rw_lock_deduct_money);
    while (1)
	{
        pthread_rwlock_rdlock(&rw_lock_deduct_money);
            int read_result=read(srcfd,c,44);
        pthread_rwlock_unlock(&rw_lock_deduct_money);
            if(read_result==0)
                break;
           //printf("%s\n",c);  
            char temp[25],temp2[25];
            strncpy(temp,data,22);  
            strncpy(temp2,c+9,22);
            if(strncmp(c+9,data,22)==0)
            {   
                char balance[11],payment[11];
                strncpy(balance,c+32,10);
                strncpy(payment,data+23,10);
                int balAmount = atoi(balance);
                int PayAmount = atoi(payment);
                if(balAmount<PayAmount)
                {
                    close(srcfd);
                    return 2; //Balance Amount is less then Money
                }else{
                    if(DeductMoney(&srcfd,balAmount-PayAmount)){
                        int ID =ReturnUTRId();
                        if(ID!=0)
                        {
                            close(srcfd);
                            return ID; //UTR return
                        }
                    }else{
                            return 0;
                    }
                }
            }
    }
    //pthread_rwlock_unlock(&rw_lock_deduct_money);
   close(srcfd);
   return 0; //Payment Does not exists
}
void* Pay(int* connfd)
 {
    size_t n;  
    char buf[MAXLINE];
    char message[50]="Bank Payment Done with Bank-UTR-ID is \0";
    char AccNOTEXIST[]="Account Details Doesn't Exists!!\0";
    char FailMess[MAXLINE]="Payment Failed due to Insufficient Balance\0";
        n = read(*connfd, buf, MAXLINE);
        printf("Bank server received %d bytes\n", (int)n);
    	buf[n] = '\0';
        printf("Bank server received message : %s\n", buf);
        int Results =Transact(buf);
        if(Results==0){
            write(*connfd,AccNOTEXIST, strlen(AccNOTEXIST));
        }else if( Results==2){
            write(*connfd,FailMess,strlen(FailMess));
        }else{
            sprintf(message+strlen(message),"%d",Results);
            write(*connfd,message,strlen(message));
        }
}
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
    pthread_mutex_init(&mymutex, NULL);
    pthread_rwlock_init(&rw_lock_deduct_money, NULL);
    while (1) {
        printf("Waiting for a new Client to connect\n");
        clientlen = sizeof(struct sockaddr_storage); /* Important! */
        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        getnameinfo((struct sockaddr *) &clientaddr, clientlen,
        client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        printf("Start Communication with Client\n");
        int *sclient=malloc(sizeof(int));
        *sclient = connfd;
        pthread_t tid;
        int err =  pthread_create(&tid,NULL,GatewayServer_handler,(void *)sclient);
        if (err != 0)
             printf("cant create thread: %s\n", strerror(err));
    }
    pthread_mutex_destroy(&mymutex);
    pthread_rwlock_destroy(&rw_lock_deduct_money);
    exit(0);
}

