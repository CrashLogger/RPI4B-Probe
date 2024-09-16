#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define MONITOR_IF "mon0"

uint8_t checkPermissions();
uint8_t checkNetworkCards(char* testString);

int main(int argc, char **argv)
{
	
	if(!checkPermissions()==0){
	  perror("\033[31m Sudo moduan exekutatu behar da fitxategi hau, barneko dependentziek sudo behar dutelako! ");
	  return(1);
	}
	
	if(!checkNetworkCards("MONITOR_IF") == 0){
	  perror("\033[93m Ez dago monitorizatzeko interfazerik ekipo honetan! ");
	  return(1);
	}
	//printf(MONITOR_IF " found!");
	
	//Pipe bat erabiliz tcpdump-en irteera gurasoari bidaliko diogu
	int pipefd[2];
	pipe(pipefd);
	
	int pid1;
	
	if((pid1 = fork()) == 0){
	  printf("Semea!\n");
	  
	  
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
