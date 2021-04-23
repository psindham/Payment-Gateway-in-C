#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
//include <netinet/in.h>
//include <arpa/inet.h>

/* Misc constants */
#define	MAXLINE	 8192  /* Max text line length */
#define LISTENQ  1024  /* Second argument to listen() */

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

int open_clientfd(char *hostname, char *port) {
    int clientfd;
    struct addrinfo hints, *listp, *p;
	char host[MAXLINE],service[MAXLINE];
    int flags;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV;  /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG;  /* Recommended for connections where we get IPv4 or IPv6 addresses */
    getaddrinfo(hostname, port, &hints, &listp);
  
    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue; /* Socket failed, try the next */

		flags = NI_NUMERICHOST | NI_NUMERICSERV; /* Display address string instead of domain name and port number instead of service name */		
		getnameinfo(p->ai_addr, p->ai_addrlen, host, MAXLINE, service, MAXLINE, flags);
        printf("host:%s, service:%s\n", host, service);
		
        /* Connect to the server */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) 
		{
			printf("Connected to server %s at port %s\n", host, service);
            break; /* Success */
		}
        close(clientfd); /* Connect failed, try another */  //line:netp:openclientfd:closefd
    } 

    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* All connects failed */
        return -1;
    else    /* The last connect succeeded */
        return clientfd;
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
int UpdateGateawyTrasaction(char buf[])
{
    char c[MAXLINE];
   size_t len = 0;
   int srcfd = open("DataFiles/PaymentTokensData.txt",O_RDWR);
    while (read(srcfd,c,29))
	{
            if(strncmp(c,buf,19)==0)
            {   
                if(c[20]=='0')
                {
                    char temp[2]="1";
                    if(lseek(srcfd,-9,SEEK_CUR)==-1)
                        printf("Error-11");
                    if(write(srcfd,temp,1)==-1)
                        printf("Error-12");
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
    while (read(srcfd,c,29))
	{
            printf("%s",c);     
            if(strncmp(c,buf,19)==0)
            {   
                if(c[20]!='0')
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

char* decode(char parameters[]){
    int childpid;
    char *result= malloc(sizeof(char)*MAXLINE);
    int fd[2];
    pipe(fd);
    if ( (childpid = fork() ) == -1){
        fprintf(stderr, "FORK failed");
    } else if( childpid == 0) {
        dup2(fd[1], 1);
        close(fd[0]);
        execlp("ShellScriptFiles/decode.sh","ShellScriptFiles/decode.sh","-c",parameters,NULL);
    }
    wait(NULL);
    read(fd[0], result, MAXLINE);
    printf("%s",result);
    return (char*)result;
}
void SaveTransaction(char UTR[],char Details[],int payment)
{
    char Resultant[50];
    memset(Resultant,0,sizeof Resultant);
    strncpy(Resultant,UTR+38,8);
    strcat(Resultant," ");
    strcat(Resultant,Details);
    sprintf(Resultant+strlen(Resultant)," %d",payment);
    strcat(Resultant,"\n");
    printf("%s",Resultant);
    int GTfd = open("DataFiles/GatewayTransaction.txt",O_APPEND|O_WRONLY);
    if(write(GTfd,Resultant,strlen(Resultant))==-1)
        printf("Error");
    close(GTfd);
}

void* PaymentTokenReceiver(int* connfd)
 {
    // memset( ,0,sizeof );
    size_t n;
    char buf[MAXLINE];
    char temp[MAXLINE];
    char *SuccessMess="Success";
    char FailMess[MAXLINE]="Payment Failed\0";
    char PaidMess[25]="Payment Already paid\0";
    int flag=1;
    while(1){
        memset(buf,0,sizeof buf);
        n = read(*connfd, buf, MAXLINE); //Read TransactionID , passcode
        if(buf[0]=='\0')
            break;
        //printf("%s",buf);
        //memset(buf,0,sizeof buf);
        //strcpy(buf,decode(buf));   
         char temp2[MAXLINE];
                strcpy(temp2,buf);
        if(buf[0]=='0')
            break;
        printf("Gateway server received %d bytes\n", (int)n);
	    buf[n] = '\0';
             printf("Gateway server received message : %s\n", buf);
             int resultPaymentValid=PaymentTokenValidator(buf);
             char temp[10]; 
             sprintf(temp,"%d",resultPaymentValid);
             printf("%d",resultPaymentValid);
            if(resultPaymentValid!=0 && resultPaymentValid!=2){ // returns the amount
                write(*connfd,temp,strlen(temp));  //Success of Payment 
                memset(buf, 0, sizeof buf);
                n = read(*connfd, buf, MAXLINE); //read userId, password
                printf("Gateway server received %d bytes\n", (int)n);
                buf[n] = '\0';
                printf("Gateway server received message : %s\n", buf);
                strcpy(temp,TransactPayment(buf));
                if(strncmp(temp,"Account",7)!=0 && strncmp(temp,"Payment",7)!=0)
                {
                    if(UpdateGateawyTrasaction(temp2)==1)
                    {
                        printf("Gateway Transaction Payment Table updated \n");
                        SaveTransaction(temp,temp2,resultPaymentValid);
                        write(*connfd,temp,strlen(temp));
                    }else{
                        printf("Gateway Transaction Payment Table Not updated \n");
                        write(*connfd,temp,strlen(temp));
                    }
                }
            }else if(resultPaymentValid==2){ //if payment is already done
                memset(buf, 0, sizeof buf);
                write(*connfd,PaidMess,strlen(PaidMess));       //Payment already done
            }else if(resultPaymentValid==0){
                memset(buf, 0, sizeof buf);
                write(*connfd,FailMess,strlen(FailMess));       //Failure of Payment
            }
    }
}
int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; /* Enough room for any addr */
    char client_hostname[MAXLINE], client_port[MAXLINE];
    listenfd = open_listenfd(argv[1]);
  
    while (1) {
        printf("Waiting for Clients to connect\n");
        clientlen = sizeof(struct sockaddr_storage); /* Important! */
        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        if(fork()==0){
            getnameinfo((struct sockaddr *) &clientaddr, clientlen,
            client_hostname, MAXLINE, client_port, MAXLINE, 0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);
            printf("Start Communication with Client\n");
            PaymentTokenReceiver(&connfd);
            printf("End Communication with Client\n");
            close(connfd);
        }
    }
    exit(0);
}

