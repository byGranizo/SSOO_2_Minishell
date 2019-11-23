#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include <signal.h>

tline * line;

void SIG_IGN_custom(int signum){
	printf("\nmsh> ");
	fflush(stdout);
}

void signalDefault(){
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
}

void signalIgnore(){
	signal(SIGINT, SIG_IGN_custom);
	signal(SIGQUIT, SIG_IGN_custom);

	/*signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);*/
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

	signalIgnore();

    printf("msh> ");
	while (fgets(buf, 1024, stdin)) {
		signalIgnore();
		
		line = tokenize(buf);
		if (line==NULL) {
			continue;
		}

		//Cambiar el pirateado, una sola funcion, con condicion OR en el if,
		//que redirija (si se le mete alguno) y si se le mete null que lo mande a lo que sea por defecto
		if (line->redirect_input != NULL) {
			
		}
		if (line->redirect_output != NULL) {
			
		}
		if (line->redirect_error != NULL) {
			
		}
		if(line->ncommands == 1){
			if(strcmp(line->commands[0].argv[0], "cd") == 0){
				changeDirectory();
			} else if(strcmp(line->commands[0].argv[0], "exit") == 0){
				exit(0);
			} else {
				instruccionNormal();
			}
			
		} else {

		}
		
		
		printf("msh> ");	
	}
}