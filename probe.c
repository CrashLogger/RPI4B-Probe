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
#define MONITOR_IF "wlp2s0"

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
      
    //Ez dugu hoditik irakurri behar semean
    close(pipefd[0]);

    //Output buffer-ak hoditik bidali
    dup2(pipefd[1], 1); //Stdout
    dup2(pipefd[1], 2); //Stderr
    
    //Stdout-ak jada konektatuta daudenez, pipefd ez dugu behar (gizakiontzat erreferentzia baino ez da)
    close(pipefd[1]);
    
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
        //tcpdump deitzen dugu, T_TEST_S segundu luzerako froga bat egiteko
        system("timeout " T_TEST_S " tcpdump -w - -U -i " MONITOR_IF " | tee home/crash/test.pcap | tcpdump -r -");
        printf("ENDED\n");
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &finish);
    
        sub_timespec(start, finish, &delta);
        char tmp[20];
        //printf("%d.%.9ld\n", (int)delta.tv_sec, delta.tv_nsec);
        sprintf(tmp, "%d.%.9ld\n", (int)delta.tv_sec, delta.tv_nsec);
        float tproc = atof(tmp);
        printf("\033[31m CAPTURE TIME: %f\n \033[39m ", tproc);
    }
    else{
    
      char buffer[255];

      while(1){
        clock_gettime(CLOCK_REALTIME, &start);
        while (read(pipefd[0], buffer, sizeof(buffer)) != 0){          

          //printf("%s", buffer);
          //fflush(stdout);
          
          clock_gettime(CLOCK_REALTIME, &finish);
    
          sub_timespec(start, finish, &delta);

          //printf("\033[31m %f\n \033[39m ", tproc);
          
          //ZERGATIK HEMEN?
          //Erlojuan lortutako datuek denbora kostatuko zaizkigu. Jada nahikoa galdu dugu aurreko lerroetan!
          //Amaieran gauden ala ez konprobatzea ere denbora kostatuko zaigu
          clock_gettime(CLOCK_REALTIME, &start);
          
          char tmp[20];
          //printf("%d.%.9ld\n", (int)delta.tv_sec, delta.tv_nsec);
          sprintf(tmp, "%d.%.9ld\n", (int)delta.tv_sec, delta.tv_nsec);
          float tproc = atof(tmp);
          
          //Bufferrekin arazoak daude. Ez dakit zergaitik oraindik. Hori konpondu arte, rafaga baten hasierako geldialdia ignoratu beharra dago
          if(tproc < 1){
            absTime = absTime + tproc;
            capturedPackets++;
          }

          if(strstr(buffer, "CAPTURE TIME")){
            printf("%s \n Bataz beste denbora/pak= %f\n", buffer, absTime/capturedPackets);
            return(0);
          }
          
        }
      }
    }
 
    return 0;
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
