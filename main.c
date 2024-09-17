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
#define T_TEST_S "30"


uint8_t checkPermissions();
uint8_t checkNetworkCards(char* testString);
double rollingCPUUsage();

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
	  //Semea, CPU neurketa
	  //Ez dugu hoditik irakurri behar semean
	  signal(SIGUSR1, cpuMeasCatcher);
	  
	  close(cpupipefd[0]);
	  
  	  //Output buffer-ak hoditik bidali
	  dup2(cpupipefd[1], 1); //Stdout
	  dup2(cpupipefd[1], 2); //Stderr
	  
	  //Stdout-ak jada konektatuta daudenez, pipefd ez dugu behar (gizakiontzat erreferentzia baino ez da)
	  close(cpupipefd[1]);
	  
	  rollingCPUUsage();
	  
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
	  //Prozesu honen lana CPU-a monitorizatzea da
	  close(pipefd[1]);
	  close(pipefd[0]);
	  close(cpupipefd[1]);
	  
	  char buffer[255];
	  char* tok;
	  
          while (read(cpupipefd[0], buffer, sizeof(buffer)) != 0)
          {
      	    if (strstr(buffer, "tcpdump") != NULL) {
              printf("\033[96m %s\n \033[39m ", buffer);
              
              tok = strtok(buffer, "\t\t");
              printf("tok1 == %s", tok);
              tok = strtok(NULL, "\t\t");
              printf("tok2 == %s", tok);
              
            }
          }	
	  
	}
	else{
	  //Gurasoa
	  printf("Gurasoa!\n");
	  
	  signal(SIGINT, exitCatcher);
	  
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
  kill(pid1, SIGKILL);
  kill(pid2, SIGKILL);
  kill(pid3, SIGKILL);
  kill(getpid(), SIGKILL);
}

void cpuMeasCatcher(){
  checkCPU = 1;
  printf("PING!!\n");
}

double rollingCPUUsage(){
  //Pakete bat jaso ostean CPU erabilera konprobatuko dugu
  while(1){
    if(checkCPU==1){
      checkCPU = 0;
      system("ps -aux | grep tcpdump");
    }
  }
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
