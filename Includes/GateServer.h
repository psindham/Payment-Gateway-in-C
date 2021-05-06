#define TRANSACTIONRECORD 29
#define ONLYTRANPASS 19
#define PAID 20
#define	MAXLINE	 8192

 int CItiBankLogFD;
 pthread_rwlock_t  rw_lock_payment;
 
int open_listenfd(char *port) ;
void * threadSaveTransaction(void *arg);
int UpdateGatewayTrasaction(char buf[]);
int PaymentTokenValidator(char buf[]);
void* PaymentTokenReceiver(int* connfd);
char* TransactPayment(char buf[]);
int open_clientfd(char *hostname, char *port);