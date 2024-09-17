#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define MONITOR_IF "mon0"

uint8_t checkPermissions();
uint8_t checkNetworkCards(char* testString);

int main(int argc, char **argv)
{
	
	if(!checkPermissions()==0){
	  perror("\033[31m Sudo moduan exekutatu behar da fitxategi hau, barneko dependentziek sudo behar dutelako! ");
	  return(1);
	}
	
	/*TODO: KOMENTARIOAK KENDU RPI-AN EXEKUTATZEAN!*/
	if(!checkNetworkCards("MONITOR_IF") == 0){
	  perror("\033[93m Ez dago monitorizatzeko interfazerik ekipo honetan! ");
	  return(1);
	}
	
	int pipefd[2];
	pipe(pipefd);
	
	int pid1;
	
	if((pid1 = fork()) == 0){
	  //Semea
	  printf("Semea!\n");
	  
	  //Ez dugu hoditik irakurri behar semean
	  close(pipefd[0]);
	  
	  //Output buffer-ak hoditik bidali
	  dup2(pipefd[1], 1); //Stdout
	  dup2(pipefd[1], 2); //Stderr
	  
	  //Stdout-ak jada konektatuta daudenez, pipefd ez dugu behar (gizakiontzat erreferentzia baino ez da)
	  close(pipefd[1]);
	  
	  execl("/usr/bin/tcpdump", "tcpdump", "-i", MONITOR_IF, NULL);
	  
	}
	else{
	  //Gurasoa
	  printf("Gurasoa!\n");
	  
	  //MTU bateko bufferra
	  char buffer[255];
	  char* tok;
	  
	  close(pipefd[1]);
	
          while (read(pipefd[0], buffer, sizeof(buffer)) != 0)
          {
          
          //TODO: Lerroak refinatu! Buffer laburragoak erabiltzea hobe(Azkarrago aterako dira->Azkarrago prozesatuko ditugu)!
          //Jasotako string-ak lerroka konpondu
            if((tok = strtok(buffer, "\n"))!=NULL){ 
              printf("%s", tok);
              while((tok = strtok(NULL, "\n"))!=NULL){
                printf("\nLINE: %s", tok);
              }
            }
            
          }
	}
	
	return 0;
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
