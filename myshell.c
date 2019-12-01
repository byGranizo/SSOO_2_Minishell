//Cuando se activan o deactivan las señales
//const del nº de mandatos maximos en bg o realloc
//dar formato a la salida de jobs

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

#include <errno.h>

#define BUFFER_SIZE 1024

char buf[BUFFER_SIZE];
tline * line;
int rIn, rOut, rErr;
pid_t * bgPidExec;
char ** bgCommandExec;
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
	if(line->commands[0].argc > 2){
		return 1;
	}

	char cwd[BUFFER_SIZE];
	if(line->commands[0].argc == 2){
		if(line->commands[0].argv[1][0] != '/'){
			getcwd(cwd,sizeof(cwd));
			strcat(cwd,"/");
			strcat(cwd, line->commands[0].argv[1]);
			chdir(cwd);
		} else {
			chdir(line->commands[0].argv[1]);
		}
	} else {
		chdir(getenv("HOME"));
	}
	printf("El nuevo directorio es: %s\n", getcwd(cwd,sizeof(cwd)));
}

int jobs(int print){
	int i,j;
	int status;
	pid_t currentPid;

	i = 0;
	while(bgPidExec[i] != 0){
		currentPid = waitpid(bgPidExec[i], &status, WNOHANG);

		if(currentPid != 0){
			for(j=i;j<50-1;j++){
				if(bgPidExec[j] != 0){
					bgPidExec[j] = bgPidExec[j+1];
					strcpy(bgCommandExec[j], bgCommandExec[j+1]);
				} else {
					bgPidExec[50-1] = 0;
				}
			}
			i--;
		} else {
			if(print){
				printf("[%d] %d - %s", i+1, bgPidExec[i], bgCommandExec[i]);	
			}
		}
		i++;
	}

	if(!print){
		printf("[%d] %d\n", i, bgPidExec[i-1]);	
	}
}

void fillJobsExecArray(pid_t pid){
	int i;
	i=0;
	while(bgPidExec[i] != 0){
		i++;
	}
	bgPidExec[i] = pid;
	strcpy(bgCommandExec[i], buf);

	jobs(0);
}

int foreground(){
	int i,j;
	int status;
	pid_t currentPid;

	if(line->commands[0].argv[1] != NULL){
		i = atoi(line->commands[0].argv[1]);
	} else {
		i = 1;
	}
	i--;

	if(bgPidExec[i] != 0){
		currentPid = waitpid(bgPidExec[i], &status, 0);
		if(currentPid != 0){
			for(j=i;j<50-1;j++){
				if(bgPidExec[j] != 0){
					bgPidExec[j] = bgPidExec[j+1];
					strcpy(bgCommandExec[j], bgCommandExec[j+1]);
				} else {
					bgPidExec[50-1] = 0;
				}
			}
		}
	}
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
			fillJobsExecArray(pid);
		} else {
			waitpid(pid, NULL, 0);
		}
		
	}
}

int pipedInstruction(){
	int i;
	int ** pipes;
	pid_t * pids;

	pipes = (int**) malloc((line->ncommands-1) * sizeof(int*));
	for(i=0;i<line->ncommands-1;i++){
		pipes[i] = (int*) malloc(2 * sizeof(int));
		pipe(pipes[i]);
	}

	pids = (pid_t*) malloc((line->ncommands) * sizeof(pid_t));
	
	for(i=0;i<line->ncommands;i++){
		signalDefault();

		pids[i] = fork();
		if (pids[i] < 0){
			//fallo
		} else if(pids[i] == 0) {
			//hijo
			if(i == 0){
				close(pipes[i][0]);
				dup2(pipes[i][1], fileno(stdout));
			} else if(i == line->ncommands - 1){
				close(pipes[i-1][1]);
				dup2(pipes[i-1][0], fileno(stdin));
			} else {
				close(pipes[i-1][1]);
				dup2(pipes[i-1][0], fileno(stdin));
				close(pipes[i][0]);
				dup2(pipes[i][1], fileno(stdout));
			}
			
			execvp(line->commands[i].argv[0], line->commands[i].argv);
			fprintf(stderr, "Error al ejecutar el comando: %s\n", strerror(errno));

			exit(1);
			
		} else {
			//padre
			if(!(i==(line->ncommands-1))){
				close(pipes[i][1]);
			}
		}
	}

	if(line->background){
		fillJobsExecArray(pids[0]);
	} else {
		for(i=0;i<line->ncommands;i++){
			waitpid(pids[i], NULL, 0);
		}
	}

	for(i=0;i<line->ncommands-1;i++){
		free(pipes[i]);
	}
	free(pipes);


}

int main(int argc){
	pid_t pid;
    int i;
	int result;

	if(argc != 1){
        return 1;
    }

	rIn = dup(fileno(stdin));
	rOut = dup(fileno(stdout));
	rErr = dup(fileno(stderr));

	bgPidExec = (pid_t*) calloc(50, sizeof(int));
	bgCommandExec = (char**) malloc(50 * sizeof(char*));
	for(i=0;i<50;i++){
		bgCommandExec[i] = (char*) malloc(BUFFER_SIZE * sizeof(char));
	}

	signalIgnore();

    printf("msh> ");
	while (fgets(buf, BUFFER_SIZE, stdin)) {
		
		
		line = tokenize(buf);
		if (line==NULL) {
			continue;
		}

		redirections();
		
		if(line->ncommands == 1){
			if(strcmp(line->commands[0].argv[0], "cd") == 0){
				changeDirectory();
			} else if(strcmp(line->commands[0].argv[0], "exit") == 0){
				for(i=0;i<50;i++){
					free(bgCommandExec[i]);
				}
				free(bgCommandExec);

				exit(0);
			} else if(strcmp(line->commands[0].argv[0], "jobs") == 0){
				jobs(1);
			} else if(strcmp(line->commands[0].argv[0], "fg") == 0){
				foreground();
			} else {
				simpleInstruction();
			}
			
		} else {
			pipedInstruction();
		}
		
		endRedirections();
		signalIgnore();
		printf("msh> ");	
	}
}