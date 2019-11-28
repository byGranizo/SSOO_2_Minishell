//Queda elegir la mejor forma de redireccion, con espacios de memoria directamente, o con ficheros/alias etc (preferible)
//Cuando se activan o deactivan las señales

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

tline * line;
int rIn, rOut, rErr;
//int redirection;

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
	int redirection;

	if (line->redirect_input != NULL) {
		printf("redirección de entrada: %s\n", line->redirect_input);

		redirection = open(line->redirect_input, O_CREAT | O_RDONLY);

		if(redirection < 0){
			return 1;
		}

		dup2(redirection, fileno(stdin));
		/*dup2(0, 7);
        dup2(redirection, 0);*/
	}
	if (line->redirect_output != NULL) {
		printf("redirección de salida: %s\n", line->redirect_output);

		redirection = creat (line->redirect_output ,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );
		//redirection = open(salida, O_WRONLY | O_CREAT | O_TRUNC);

		if(redirection < 0){
			return 1;
		}

		dup2(redirection, fileno(stdout));
		/*dup2(1, 8);
        dup2(redirection, 1);*/
	}
	if (line->redirect_error != NULL) {
		printf("redirección de error: %s\n", line->redirect_error);

		redirection = creat (line->redirect_error ,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		//redirection = open(salida, O_WRONLY | O_CREAT | O_TRUNC);

		if(redirection < 0){
			return 1;
		}

		dup2(redirection, fileno(stderr));

		/*dup2(2, 9);
        dup2(redirection, 2);*/
	}
}

int endRedirections(){
	if(line->redirect_input != NULL ){
			dup2(rIn , fileno(stdin));

			/*close(redirection);
            dup2(7, 0);*/
		}
		if(line->redirect_output != NULL ){
			dup2(rOut , fileno(stdout));	

			/*close(redirection);
            dup2(8, 1);*/
		}
		if(line->redirect_error != NULL ){
			dup2(rErr , fileno(stderr));

			/*close(redirection);
            dup2(9, 2);*/	
		}
}

int simpleInstruction(){
	pid_t pid;
	pid = fork();
	if (pid < 0){
		//fallo
	} else if(pid == 0) {
		//hijo
		execvp(line->commands[0].argv[0], line->commands[0].argv);


		exit(1);
	} else {
		//padre
		if(line->background){
			printf("[%d]\n",pid);
		} else {
			waitpid(pid, NULL, 0);
		}
		
	}
}

int pipedInstruction(){
	int i;
	int ** pipes;
	pid_t pid;

	pipes = (int**) malloc((line->ncommands-1) * sizeof(int*));
	for(i=0;i<line->ncommands-1;i++){
		pipes[i] = (int*) malloc(2 * sizeof(int));
	}

	for(i=0;i<line->ncommands-1;i++){
		pipe(pipes[i]);
	}

	for(i=0;i<line->ncommands;i++){
		pid = fork();
		if (pid < 0){
			//fallo
		} else if(pid == 0) {
			//hijo
			if(i == 0){
				close(pipes[i][0]);
				dup2(pipes[i][1], fileno(stdout));
			} else if(i == line->ncommands - 1){
				close(pipes[i-1][1]);
				dup2(pipes[i-1][1], fileno(stdin));
			} else {
				close(pipes[i-1][1]);
				dup2(pipes[i-1][0], fileno(stdin));
				
				close(pipes[i][0]);
				dup2(pipes[i][1], fileno(stdout));
			}
			
			execvp(line->commands[0].argv[0], line->commands[0].argv);

			exit(1);
			
		} else {
			//padre
			
			
		}
	}

	for(i=0;i<line->ncommands-1;i++){
		free(pipes[i]);
	}
	free(pipes);


}

int main(int argc){
    char buf[1024];
	pid_t pid;
    int i;
	int result;

	if(argc != 1){
        return 1;
    }

	rIn = dup(fileno(stdin));
	rOut = dup(fileno(stdout));
	rErr = dup(fileno(stderr));

	signalIgnore();

    printf("msh> ");
	while (fgets(buf, 1024, stdin)) {
		signalIgnore();
		
		line = tokenize(buf);
		if (line==NULL) {
			continue;
		}

		redirections();
		
		if(line->ncommands == 1){
			if(strcmp(line->commands[0].argv[0], "cd") == 0){
				changeDirectory();
			} else if(strcmp(line->commands[0].argv[0], "exit") == 0){
				exit(0);
			} else {
				simpleInstruction();
			}
			
		} else {
			pipedInstruction();
		}
		
		endRedirections();
		printf("msh> ");	
	}
}