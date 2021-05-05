#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
 
int main(void) {
  int number1, number2, sum;
 
  int input_fds = open("./input.txt", O_RDONLY);
 
  if(dup2(input_fds, STDIN_FILENO) < 0) {
    printf("Unable to duplicate file descriptor.");
    exit(EXIT_FAILURE);
  }
 
  scanf("%d %d", &number1, &number2);
 
  sum = number1 + number2;
 
  printf("%d + %d = %d\n", number1, number2, sum);
 
  return EXIT_SUCCESS;
}