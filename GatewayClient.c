#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include<sys/wait.h>
//include <netinet/in.h>
//include <arpa/inet.h>

#define	MAXLINE	 8192  /* Max text line length */

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

char* encode(char parameters[]){
    int childpid;
    char *result= malloc(sizeof(char)*MAXLINE);
    int fd[2];
    pipe(fd);
    if ( (childpid = fork() ) == -1){
        fprintf(stderr, "FORK failed");
        
    } else if( childpid == 0) {
        dup2(fd[1], 1);
        close(fd[0]);
        execlp("shellScriptFiles/encode.sh","shellScriptFiles/encode.sh","-c",parameters,NULL);
    }
    wait(NULL);
    read(fd[0], result, MAXLINE);
    return (char*)result;
}

int main(int argc, char **argv)
{
    int clientfd,optionToConti=1;
    char *host, *port, buf[MAXLINE];
    host = argv[1];
    port = argv[2];
    clientfd = open_clientfd(host, port);

    char TransactionID[13],Passcode[8],UserID[20],Password[15];
    while(optionToConti){
        memset(TransactionID,0,sizeof TransactionID);
        memset(Passcode ,0,sizeof Passcode);
        memset(UserID ,0,sizeof UserID);
        memset(Password ,0,sizeof Password);
        
        memset(buf,0,sizeof buf);
        printf("*****************WELCOME*********************\n");
        printf("Enter Transaction ID and PassCode for Payment\n");
        printf("*********************************************\n");
        scanf("%s",TransactionID);
        scanf("%s",Passcode);
        strcpy(buf,TransactionID);
        strcat(buf," ");
        strcat(buf,Passcode);
        //   strcpy(buf,encode(buf));
        write(clientfd, buf, strlen(buf));
        memset(buf, 0, sizeof buf);
        read(clientfd, buf, MAXLINE);
        if(strcmp(buf,"Payment Failed")==0||strcmp(buf,"Payment Already paid")==0 ){
            fputs(buf, stdout);
        }
        else{
            printf("Amount to be paid is %s for Transaction ID %s\n\n",buf,TransactionID);
            printf("*********************************************\n");
            printf("   Enter USER  ID and Password for Payment\n");
            printf("*********************************************\n");
            scanf("%s",UserID);
            scanf("%s",Password);
            strcpy(buf,"");
            strcpy(buf,UserID);
            strcat(buf," ");
            strcat(buf,Password);
            strcat(buf," ");
            write(clientfd, buf, strlen(buf));
            memset(buf, 0, sizeof buf);
            read(clientfd, buf, MAXLINE);
            fputs(buf, stdout);
            memset(buf, 0, sizeof buf);
    }
    printf("\n\n Enter 0 to EXIT 1 for other Payments : ");
    scanf("%d",&optionToConti);
    }
    buf[0]='0';
    write(clientfd, buf,1);
    close(clientfd);
    exit(0);
}
