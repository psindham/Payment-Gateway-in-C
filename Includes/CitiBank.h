#define	MAXLINE	 8192  /* Max text line length */
#define LISTENQ  1024  /* Second argument to listen()*/
#define USERDETAILS 22 /* Only Username Password */
#define USERRECORD 44  /* Entire DATA(RECORD) to read */

 pthread_rwlock_t rw_lock_Transact_money;
 pthread_rwlock_t rw_lock_credit_money;
 pthread_mutex_t mymutex;

int open_listenfd(char *port);
void * threadSaveTransaction(void *arg);
int* getFDMerchantFile(char MerchantId[]);
int DeductMoney(int *srcfd,int RemainingAmount,char MerchantId[],int PayAmount);
int Transact(char data[]);


