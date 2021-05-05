struct params
{
	char UTR[50];
    char Details[50];
    int payment;
}typedef params;

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
    int GTfd = open("DataFiles/GatewayTransaction.txt",O_APPEND|O_WRONLY);
    if(write(GTfd,Resultant,strlen(Resultant))==-1)
        printf("Error");
    close(GTfd);
    pthread_exit(0);
}