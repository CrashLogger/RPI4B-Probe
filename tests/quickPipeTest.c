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
#include <wait.h>

#define SIZEOFBUFFER 1024


int main(int argc, char **argv)
{
	
	int status = 0;
	uint32_t pid1 = 0;
	
	int pipefd[2];
    pipe(pipefd);
    
    char buffer[SIZEOFBUFFER];
	
	
	if((pid1 = fork())==0){
		
		//Ez dugu hoditik irakurri behar semean
		close(pipefd[0]);

		//Output buffer-ak hoditik bidali
		dup2(pipefd[1], 1); //Stdout
		dup2(pipefd[1], 2); //Stderr
	
		close(pipefd[1]);
		
		//system("timeout 25 tcpdump -i lo");
		
		printf("Starting:\n");
		execl("/usr/bin/timeout", "timeout", "20", "tcpdump", "-i", "lo", "-v", NULL);
		
		
	}
	else{
		
		//Ez dugu hodira idatzi behar gurasoan
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		
		while(1){
			fgets(buffer, SIZEOFBUFFER, stdin);
			
			if(strlen(buffer)>0){
				printf("Buffer: \t %s\n", buffer);
				buffer[0] = 0;
			}
		}
		wait(&status);
	}
	
	return 0;
}

