#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

void exitCatcher();
float getCPU(char* string);

double absCPUUsage = 0.0;
int CPUSamples = 0;

int pid1 = 0;

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
	  
	  close(cpupipefd[1]);
	  
	  while(1){
	    sleep(1);
	    printf("Child: %d\n", getpid());
	    system("ps -aux | grep wireshark");
	  }
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
              printf("%s\n", line0);
              line1 = strtok(NULL, "\n");
              printf("%s\n", line1);
              line2 = strtok(NULL, "\n");
              printf("%s\n", line2);
              
              absCPUUsage = absCPUUsage + getCPU(line0) + getCPU(line1) + getCPU(line2);
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
  return(atof(tok));
}

void exitCatcher(){
  printf("\n%f -eko CPU erabilera bataz beste.\n", absCPUUsage/CPUSamples);
  kill(pid1, SIGKILL);
  kill(getpid(), SIGKILL);
}
