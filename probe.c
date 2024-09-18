#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>

#define T_TEST_S "240"
#define MONITOR_IF "mon0"

enum { NS_PER_SECOND = 1000000000 };

void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td);

double absTime = 0.0;
double absCPU = 0.0;

int capturedPackets = 0;
int pid1;

uint8_t done = 0;

int main(int argc, char **argv)
{

  struct timespec start, finish, delta;
  struct timespec cpuStart, cpuFinish, cpuDelta;

  int pipefd[2];
  pipe(pipefd);
  
  if((pid1 = fork()) == 0){
    //Semea, hemen (honen azpian berez, system erabiltzen delako eta es execl) tcpdump exekutatuko da
    
    //Ez dugu hoditik irakurri behar semean
    close(pipefd[0]);

    //Output buffer-ak hoditik bidali
    dup2(pipefd[1], 1); //Stdout
    dup2(pipefd[1], 2); //Stderr
    
    //Stdout-ak jada konektatuta daudenez, pipefd ez dugu behar (gizakiontzat erreferentzia baino ez da)
    close(pipefd[1]);
  
    //tenporizadorea martxan jartzen dugu, paketeko denbora izateko
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    
    //tcpdump abiarazten dugu, T_TEST_S segundu luzerako froga bat egiteko
    system("timeout " T_TEST_S " tcpdump -w - -U -i " MONITOR_IF " | tee home/crash/test.pcap | tcpdump -r -");
    printf("ENDED\n");
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &finish);

    //Erlojuaren kaptura amaitu denez, eta beraz kaptura ere, inefizientzia onargarria da atal honetan.
    sub_timespec(start, finish, &delta);
    char tmp[20];
    //printf("%d.%.9ld\n", (int)delta.tv_sec, delta.tv_nsec);
    sprintf(tmp, "%d.%.9ld\n", (int)delta.tv_sec, delta.tv_nsec);
    float tproc = atof(tmp);
    printf("\033[31m CAPTURE TIME: %.12lf\n CAPTURE TIME PER PACKET: %.12lf \033[39m ", tproc, tproc/capturedPackets);
  }
  else{
    //Gurasoa, hemen estatistikak egiten dira.
    char buffer[255];

    while(1){
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
      
      //Hoditik jasotako gauzak
      while (read(pipefd[0], buffer, sizeof(buffer)) != 0){          
        capturedPackets++;

        if(strstr(buffer, "CAPTURE TIME")){
          clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &finish);
          
          char tmp[20];
          sprintf(tmp, "%d.%.9ld\n", (int)delta.tv_sec, delta.tv_nsec);
          float tproc = atof(tmp);
          absTime = tproc;
          printf("%s \nPakete kopurua: %d \n Bataz beste denbora/pak= %.9lf \n", buffer, capturedPackets, absTime/capturedPackets);
          return(0);
        }
        
      }
    }
  }
 
  return 0;
}

uint8_t T2Sim(uint32_t usecKop){

  struct timespec start, check;

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

  while(1){
  
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &check);
    if(((uint64_t)check.tv_nsec - (uint64_t)start.tv_nsec) > usecKop * 1000){
      return(0);
    }
    else{
      //Ezer ez dugu egiten
    }
  
  }

}

void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td)
{
    td->tv_nsec = t2.tv_nsec - t1.tv_nsec;
    td->tv_sec  = t2.tv_sec - t1.tv_sec;
    if (td->tv_sec > 0 && td->tv_nsec < 0)
    {
        td->tv_nsec += NS_PER_SECOND;
        td->tv_sec--;
    }
    else if (td->tv_sec < 0 && td->tv_nsec > 0)
    {
        td->tv_nsec -= NS_PER_SECOND;
        td->tv_sec++;
    }
}
