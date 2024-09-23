#include <stdio.h>
#include <time.h>
#include <stdint.h>

void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td);

int main(int argc, char **argv)
{
  
  long int waited = 0;
  struct timespec start, finish, check;

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

  while(1){
  
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &check);
    if(((uint64_t)check.tv_nsec - (uint64_t)start.tv_nsec) > 18000){
      printf("%ld - %ld - Bosh\n", (uint64_t)check.tv_nsec - (uint64_t)start.tv_nsec, waited);
      waited = 0;
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    }
    else{
      //printf("\n  -  %ld", ((uint64_t)check.tv_nsec - (uint64_t)start.tv_nsec));
      waited++;
    }
  
  }

  return 0;
}
