#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include "parser.h"
#include <signal.h>


void SIGINT_custom(int signum){
	printf("\nmsh> ");
	fflush(stdout);
}

void SIGQUIT_custom(int signum){
	printf("\nmsh> ");
	fflush(stdout);
}

int instruccionNormal(tline * line){
	pid_t pid;
	pid = fork();
	if (pid < 0){
		//fallo
	} else if(pid == 0) {
		//hijo
		execvp(line->commands[0].argv[0], line->commands[0].argv);
		exit(0);
	} else {
		//padre
		if(line->background){
			printf("[%d]\n",pid);
		} else {
			waitpid(pid, NULL, 0);
		}
		
	}
}

int changeDirectory(){

}

int pipes(){

}

int redirections(){

}

int main(int argc){
    char buf[1024];
	tline * line;
	pid_t pid;
    int i;
	int result;

	if(argc != 1){
        return 1;
    }

	signal(SIGINT, SIGINT_custom);
	signal(SIGQUIT, SIGQUIT_custom);

    printf("msh> ");
	while (fgets(buf, 1024, stdin)) {
		signal(SIGINT, SIGINT_custom);
		signal(SIGQUIT, SIGQUIT_custom);
		
		line = tokenize(buf);
		if (line==NULL) {
			continue;
		}

		if(line->ncommands == 1){
			if (line->redirect_input != NULL) {
			
			}
			if (line->redirect_output != NULL) {
				
			}
			if (line->redirect_error != NULL) {
				
			}
			instruccionNormal(line);
		} else {

		}
		
		
		printf("msh> ");	
	}
}