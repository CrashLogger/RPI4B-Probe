#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <sys/stat.h>
#include <signal.h>

#define MONITOR_IF "mon0"

//Luis-ek duela urte asko 4 minutuko test denbora hartu zuen, erreferentzia izateko balioko zaigu
#define T_TEST_S "5"


uint8_t checkPermissions();
uint8_t checkNetworkCards(char* testString);
float getCPU(char* string);

double absCPUUsage = 0.0;
int CPUSamples = 0;

void exitCatcher();

//Seinalez eta aldagai honetaz baliatuz jakingo dugu noiz konprobatu CPU erabilera
void cpuMeasCatcher();
uint8_t checkCPU = 0;

int pid1, pid2, pid3;

int main(int argc, char **argv)
{
	
	if(!checkPermissions()==0){
	  perror("\033[31m Sudo moduan exekutatu behar da fitxategi hau, barneko dependentziek sudo behar dutelako! \033[39m ");
	  return(1);
	}
	
	/*TODO: KOMENTARIOAK KENDU RPI-AN EXEKUTATZEAN!*/
	if(!checkNetworkCards("MONITOR_IF") == 0){
	  perror("\033[93m Ez dago monitorizatzeko interfazerik ekipo honetan! \033[39m");
	  return(1);
	}
	
	//Raspberry-an gaudenean eta konfiguratu dugunean, errore honen aurrean behar diren terminal komandoak:
	/*
	iw phy `iw dev wlan0 info | gawk '/wiphy/ {printf "phy" $2}'` interface add mon0 type monitor
	ifconfig mon0 up
	*/
	
	int pipefd[2];
	pipe(pipefd);
	
        int cpupipefd[2];
	pipe(cpupipefd);
	
	if((pid2 = fork()) == 0){
	  //Semea
	  close(cpupipefd[0]);
	  
	  dup2(cpupipefd[1], 1); //Stdout
	  dup2(cpupipefd[1], 2); //Stderr
	  
	  close(cpupipefd[1]);
	  
	  while(1){
	    sleep(1);
	    printf("Child: %d\n", getpid());
	    system("ps -aux | grep tcpdump");
	  }
	}
	else if((pid1 = fork()) == 0){
	  //Semea, tcpdump
	  printf("Semea!\n");
	  
	  //Ez dugu hoditik irakurri behar semean
	  close(pipefd[0]);
	  
	  //Output buffer-ak hoditik bidali
	  dup2(pipefd[1], 1); //Stdout
	  dup2(pipefd[1], 2); //Stderr
	  
	  //Stdout-ak jada konektatuta daudenez, pipefd ez dugu behar (gizakiontzat erreferentzia baino ez da)
	  close(pipefd[1]);
	  
	  // "home/harrapaketak" azpi karpeta dagoen ala ez konprobatu, eta existitzen ez bada egin
          mkdir("/home/harrapaketak", 777);
	  
	  system("timeout " T_TEST_S " tcpdump -w - -U -i " MONITOR_IF " | tee home/harrapaketak/test.pcap | tcpdump -r -");
	  
	  //execl("/usr/bin/tcpdump", "tcpdump", "-i", MONITOR_IF, NULL);
	  
	}
	else if((pid3 = fork()) == 0){
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
          return 0;
	}
	else{
	  //Gurasoa
	  printf("Gurasoa!\n");
	  
          signal(SIGINT, exitCatcher);
	  pause;
	  
	  //MTU bateko bufferra
	  char buffer[255];
	  char* tok;
	  
	  close(pipefd[1]);
	  close(cpupipefd[0]);
	  close(cpupipefd[1]);  
	
          while (read(pipefd[0], buffer, sizeof(buffer)) != 0)
          {
          
          if ((strstr(buffer, "packets captured") != NULL) || (strstr(buffer, "packets received") != NULL) || (strstr(buffer, "packets dropped") != NULL)) {
          
            printf("\n \033[92m %s \033[39m", buffer);
          
          }
          
          //TODO: Lerroak refinatu! Buffer laburragoak erabiltzea hobe(Azkarrago aterako dira->Azkarrago prozesatuko ditugu)!
          //Jasotako string-ak lerroka konpondu
            if((tok = strtok(buffer, "\n"))!=NULL){ 
              printf("%s", tok);
              while((tok = strtok(NULL, "\n"))!=NULL){
                printf("\033[39m \nLINE: %s", tok);
              }
            }
            kill(pid2, SIGUSR1);
          }
          
	}
	
	return 0;
}

void exitCatcher(){
  printf("\n%f -eko CPU erabilera bataz beste.\n", absCPUUsage/CPUSamples);
  kill(pid1, SIGKILL);
  kill(pid2, SIGKILL);
  kill(pid3, SIGKILL);
  kill(getpid(), SIGKILL);
}

float getCPU(char* string){
  char* tok = 0;
  tok = strtok(string, "  ");
  tok = strtok(NULL, "  ");
  tok = strtok(NULL, "  ");
  return(atof(tok));
}

void cpuMeasCatcher(){
  checkCPU = 1;
  printf("PING!!\n");
}

uint8_t checkPermissions(){
  if(geteuid()){
    errno = EPERM;
    return(1);
  }
  return(0);
}

uint8_t checkNetworkCards(char* testString){
  if(!system("ifconfig | grep " MONITOR_IF)){
    return(0);
  }
  else{
    errno = ENODEV;
    return(1);
  }
}
