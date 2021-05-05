#define  MAXLINE 8192
  
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
   // printf("%s",result);
    return (char*)result;
}