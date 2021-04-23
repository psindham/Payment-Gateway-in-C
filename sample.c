#include<stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/stat.h>
#include<stdlib.h>
int main(){
    int TransFD = open("DataFiles/TransactionID.txt",O_RDWR);
    unsigned int ID ;
    char bufferOFDATA[10];
    read(TransFD,bufferOFDATA,8);
    ID=atoi(bufferOFDATA);
    int previousID =ID;
    int NextID =++ID;
    printf("\n%d\n",NextID);
    if(lseek(TransFD,-8,SEEK_CUR)==-1)
        printf("Error-11");
    sprintf(bufferOFDATA,"%d",NextID);
    if(write(TransFD,bufferOFDATA,8)==-1)
         printf("Error-12");
    close(TransFD);
    return 0;
}
