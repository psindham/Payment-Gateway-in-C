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
#include <GateServer.h>

struct params{
	char UTR[50];
    char Details[50];
    int payment;
}typedef params;


char* TransactPayment(char buf[]){
    
    int clientfd;
    char *host="localhost", *port="15001";    //Bank server is running on the PORT no. 15001 
    clientfd = open_clientfd(host, port);     //create a client FD to communicate with bank 
    write(clientfd, buf, strlen(buf));
    read(clientfd, buf, MAXLINE);
    close(clientfd);
    return (char *)buf;
}
void * threadSaveTransaction(void *arg)
{
    struct params *thread_p = arg;
    char Resultant[50];
    memset(Resultant,0,sizeof Resultant);
    strncpy(Resultant,thread_p->UTR+38,8);
    strcat(Resultant," ");
    strcat(Resultant,thread_p->Details);
    sprintf(Resultant+strlen(Resultant)," %d",thread_p->payment);
    strcat(Resultant,"\n");
   // printf("%s",Resultant);
    int GTfd = open(".././DataFiles/GatewayTransaction.txt",O_APPEND|O_WRONLY);
    if(write(GTfd,Resultant,strlen(Resultant))==-1)
        printf("Error");
    close(GTfd);
    pthread_exit(0);
}


int UpdateGatewayTrasaction(char buf[])
{
   char c[MAXLINE];
   size_t len = 0;
   int srcfd = open(".././DataFiles/PaymentTokensData.txt",O_RDWR);
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
   int srcfd = open(".././DataFiles/PaymentTokensData.txt",O_RDONLY);
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
