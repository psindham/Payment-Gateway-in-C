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
#include <CitiBank.h>

/*Provide UTR(Unique Transaction number) NUMBER */

void * threadReturnUTRId(void *arg)
{
    int TransFD = open(".././DataFiles/TransactionID.txt",O_RDWR);
    unsigned int ID ;
    unsigned int *ret1 = (unsigned int *)malloc(sizeof(unsigned int));
    char bufferOFDATA[10];
    memset(bufferOFDATA, 0, sizeof bufferOFDATA);
        pthread_mutex_lock(&mymutex);
            read(TransFD,bufferOFDATA,8);
            ID=atoi(bufferOFDATA);
            *ret1 =ID;
            int NextID =++ID;
            if(lseek(TransFD,-8,SEEK_CUR)==-1)
            {
                printf("Error-SEEK in File TransactionID.txt\n");
                return ((void *)  0);
            }
            sprintf(bufferOFDATA,"%d",NextID);
            if(write(TransFD,bufferOFDATA,8)==-1)
            {
                printf("Error-WRITE in File TransactionID.txt\n");
                return ((void *)  0);
            }   
            close(TransFD);
        pthread_mutex_unlock(&mymutex);
    return ((void *) ret1);
}



int* getFDMerchantFile(char MerchantId[])
{
    char c[LISTENQ];
    size_t len = 0;
    int srcfd = open(".././DataFiles/Merchants.txt", O_RDWR);
    int *ptr=&srcfd;
    while (1)
	{
        pthread_rwlock_rdlock(&rw_lock_credit_money);
            int read_result=read(srcfd,c,20);
        pthread_rwlock_unlock(&rw_lock_credit_money);
            if(read_result==0)
                break;
           printf("%s %s\n",c,MerchantId);  
            if(strncmp(c,MerchantId,4)==0)
            {
                     return ptr;
            }
    }
    return 0;
}


int DeductMoney(int *srcfd,int RemainingAmount,char MerchantId[],int PayAmount)
{
    char buf[15];
    sprintf(buf,"%010d%%",RemainingAmount);

    int *desfd = getFDMerchantFile(MerchantId);
    lseek(*desfd,-15,SEEK_CUR);
    char MerchantAmount[15];
    read(*desfd,MerchantAmount,13);
    long int amt = atoi(MerchantAmount);
    amt=amt+PayAmount;

    sprintf(MerchantAmount,"%013ld%%",amt);

    if(lseek(*srcfd,-12,SEEK_CUR)==-1 || lseek(*desfd,-13,SEEK_CUR)==-1)
    {
        printf("Error-SEEK in File NetBankingUsers.txt\n");
        return 0;
    }

    pthread_rwlock_wrlock(&rw_lock_Transact_money);
            if(write(*srcfd,buf,10)==-1 || write(*desfd,MerchantAmount,13) == -1)
            {
                printf("Error-WRITE in File NetBankingUsers.txt\n");
                return 0;
            }
    pthread_rwlock_unlock(&rw_lock_Transact_money);
    return 1;
}

/* Uses Another Thread to Do work*/

int Transact(char data[])
{
    char c[LISTENQ];
    size_t len = 0;
    int srcfd = open(".././DataFiles/NetBankingUsers.txt", O_RDWR);
  
    while (1)
	{
        pthread_rwlock_rdlock(&rw_lock_Transact_money);
            int read_result=read(srcfd,c,USERRECORD);
        pthread_rwlock_unlock(&rw_lock_Transact_money);
            if(read_result==0)
                break;
           printf("%s\n",c);  
            if(strncmp(c+9,data,USERDETAILS)==0)
            {   
                char balance[11],payment[11];
                strncpy(balance,c+32,10);
                strncpy(payment,data+28,10);
                int balAmount = atoi(balance);
                int PayAmount = atoi(payment);
                // printf("%d %d %s",balAmount,PayAmount,data);

                if(balAmount<PayAmount)
                {
                    close(srcfd);
                    return 2; //Balance Amount is less then Money
            
                }else{
                    char MerchantId[7];
                    strncpy(MerchantId,data+24,4);
                    if(DeductMoney(&srcfd,balAmount-PayAmount,MerchantId,PayAmount)){
                  
                            pthread_t threadID;
                            /* Uses Another Thread to Do work*/
                            int err = pthread_create (&threadID,NULL,threadReturnUTRId,NULL);
                            if (err != 0)
                                printf("cant create thread: %s\n", strerror(err));
                            
                            unsigned int *ID; // return ID

                            err = pthread_join(threadID, (void **)&ID);
                            if (err != 0)
                                printf("cant join with thread1: %s\n", strerror(err));
                                //int ID =ReturnUTRId(); //Added Thread instead
                                if(((unsigned int)(*ID))!=0)
                                {
                                    close(srcfd);
                                    return (unsigned int)(*ID); //UTR return
                                }else{
                                    return 0; //should be dummy UTR
                                }

                    }else{
                            return 0;
                    }
                }
            }
    }
   close(srcfd);
   return 0; //Payment Does not exists
}