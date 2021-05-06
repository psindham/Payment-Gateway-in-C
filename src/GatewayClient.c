#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include ".././includes/header.h" // For Encoding 

#define	MAXLINE	 8192  /* Max text line length */

int stdin_dup;
  
void alarmHandler(){
    printf("\n\t\tSession Expired..\n");
 return;
}

void (*oldhandler)();

int open_clientfd(char *hostname, char *port);

int main(int argc, char **argv)
{
    int clientfd,optionToConti=1;
    char *host, *port, buf[MAXLINE];
    host = argv[1];
    port = argv[2];
    clientfd = open_clientfd(host, port);

     stdin_dup= dup(STDIN_FILENO);
  struct sigaction sa;
  sa.sa_handler = alarmHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGALRM, &sa, NULL);

    //signal( SIGALRM, alarmHandler );	/* Install signal handler*/

    char TransactionID[13],Passcode[8],UserID[20],Password[15];
    while(optionToConti){
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
            write(clientfd, buf, strlen(buf));
            memset(buf, 0, sizeof buf);
            read(clientfd, buf, MAXLINE);
            if(strcmp(buf,"Payment Failed")==0||strcmp(buf,"Payment Already paid")==0 ){
                printf("\n");
                fputs(buf, stdout);
            }else{

                alarm(10);	/* Schedule an alarm signal in 10 seconds */
                
                printf("\nAmount to be paid is %s for Transaction ID %s\n\n",buf,TransactionID);
                printf("*********************************************\n");
                printf("   Enter USER  ID and Password for Payment\n");
                printf("*********************************************\n");
                if (scanf("%s",UserID)==1 && scanf("%s",Password)==1){
                    // scanf("%s",UserID);
                    // scanf("%s",Password);
                    alarm(0);
                    strcpy(buf,"");
                    strcpy(buf,UserID);
                    strcat(buf," ");
                    strcat(buf,Password);
                    strcat(buf," ");
                    // strcpy(buf,encode(buf));
                    write(clientfd, buf, strlen(buf));
                    memset(buf, 0, sizeof buf);
                    read(clientfd, buf, MAXLINE);
                    if(strncmp(buf,"Bank",4)==0){
                        printf("\n******************Congrats*******************\n");
                    }
                    fputs(buf, stdout);
                    memset(buf, 0, sizeof buf);
                }
                
                
        }
        // sigemptyset(&sa.sa_mask);
                 memset(buf, 0, sizeof buf);
                 strcpy(buf,"DUMMY");
                 write(clientfd, buf, strlen(buf));
                 read(clientfd, buf, MAXLINE);
            printf("\n\n Enter 0 to EXIT 1 for other Payments : ");
            scanf("%d",&optionToConti);
        }
    }
    buf[0]='0';
    write(clientfd,buf,1);
    close(clientfd);
    exit(0);
}
