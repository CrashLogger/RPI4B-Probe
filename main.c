#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>


uint8_t checkNetworkCards(char* testString);

int main(int argc, char **argv)
{
	
	checkNetworkCards("mon0");
	
	return 0;
}

uint8_t checkNetworkCards(char* testString){
  if(!system("ifconfig | grep mon0")){
    printf("Monitor interface found!\n");
    return(0);
  }
  else{
    perror("No monitor interface found, did ifconfig work?\nIfconfig returned");
    return(1);
  }
}
