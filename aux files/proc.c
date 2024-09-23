#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MONITOR_IF "mon0"

//Luis-ek duela urte asko 4 minutuko test denbora hartu zuen, erreferentzia izateko balioko zaigu
#define T_TEST_S "5"

void exitCatcher();
float getCPU(char* string);

double absCPUUsage = 0.0;
int CPUSamples = 0;

int pid1 = 0;
int pid2 = 0;
int pid3 = 0;

int main(int argc, char **argv)
{
	
	int pid = getpid();

        int cpupipefd[2];
	pipe(cpupipefd);
	
	if((pid1 = fork()) == 0){
	  //Semea
	  close(cpupipefd[0]);
	  
	  dup2(cpupipefd[1], 1); //Stdout
	  dup2(cpupipefd[1], 2); //Stderr
	  printf("Here!");
	  close(cpupipefd[1]);
	  
	  while(1){
	    sleep(1);
	    printf("Child: %d\n", getpid());
	    system("ps -aux | grep wireshark");
	  }
	}
	else if((pid2 = fork()) == 0){
	  /*close(pipefd[0]);
	  
	  //Output buffer-ak hoditik bidali
	  dup2(pipefd[1], 1); //Stdout
	  dup2(pipefd[1], 2); //Stderr
	  
	  //Stdout-ak jada konektatuta daudenez, pipefd ez dugu behar (gizakiontzat erreferentzia baino ez da)
	  close(pipefd[1]);
	  
	  // "home/harrapaketak" azpi karpeta dagoen ala ez konprobatu, eta existitzen ez bada egin
          mkdir("/home/harrapaketak", 777);
	  
	  system("timeout " T_TEST_S " tcpdump -w - -U -i " MONITOR_IF " | tee home/harrapaketak/test.pcap | tcpdump -r -");
	  
	  //execl("/usr/bin/tcpdump", "tcpdump", "-i", MONITOR_IF, NULL);
	  */
	}
	else if((pid3 = fork()) == 0){
	  return 0;
	}
	else{
	  signal(SIGINT, exitCatcher);
	  pause;
	  char buffer[1500];
	  char* line0;
	  char* line1;
	  char* line2;
	
	  //Gurasoa
	  close(cpupipefd[1]);
          while(1){
            while (read(cpupipefd[0], buffer, sizeof(buffer)) != 0)
            {
              printf("Parent: %d\n", getpid());
              line0 = strtok(buffer, "\n");
              if(line0 != NULL){
                printf("%s\n", line0);
                absCPUUsage = absCPUUsage + getCPU(line0);
              }
              printf("%s\n", line0);
              line1 = strtok(NULL, "\n");
              if(line1 != NULL){
                printf("%s\n", line1);
                absCPUUsage = absCPUUsage + getCPU(line1);
              }
              printf("%s\n", line1);
              line2 = strtok(NULL, "\n");
              if(line2 != NULL){
                printf("%s\n", line2);
                absCPUUsage = absCPUUsage + getCPU(line2);
              }
              CPUSamples++;
              
            }
          }
	}
	
	return 0;
}

float getCPU(char* string){
  char* tok = 0;
  tok = strtok(string, "  ");
  tok = strtok(NULL, "  ");
  tok = strtok(NULL, "  ");
  printf("%f\n", atof(tok));
  return(atof(tok));
}

void exitCatcher(){
  printf("\n%f -eko CPU erabilera bataz beste.\n", absCPUUsage/CPUSamples);
  kill(pid1, SIGKILL);
  kill(getpid(), SIGKILL);
}
