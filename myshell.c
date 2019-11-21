#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include <signal.h>

tline * line;

void SIGINT_custom(int signum){
	printf("\nmsh> ");
	fflush(stdout);
}

void SIGQUIT_custom(int signum){
	printf("\nmsh> ");
	fflush(stdout);
}

int instruccionNormal(){
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
	if(line->commands[0].argc != 2){
		return 1;
	}

	char cwd[1024];
	if(line->commands[0].argv[1][0] != '/'){
		getcwd(cwd,sizeof(cwd));
        strcat(cwd,"/");
        strcat(cwd, line->commands[0].argv[1]);
        chdir(cwd);
	} else {
		chdir(line->commands[0].argv[1]);
	}
	printf("El nuevo directorio es: %s\n", getcwd(cwd,sizeof(cwd)));
}

int redirections(){

}

int pipes(){

}

int main(int argc){
    char buf[1024];
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
			if(strcmp(line->commands[0].argv[0], "cd") == 0){
				changeDirectory();
			} else {
				instruccionNormal();
			}
			
		} else {

		}
		
		
		printf("msh> ");	
	}
}