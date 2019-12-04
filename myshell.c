#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parser.h"

#include <errno.h>

#define BUFFER_SIZE 1024

char buf[BUFFER_SIZE];
tline * line; //Stores the information required of the user instruction
int rIn, rOut, rErr; //Stores defult stdin, stdout and std err, to restore after a command is executed 
pid_t * bgPidExec; //Dynamically stores the PIDs executing in background
char ** bgCommandExec; //Dynamically stores the user instructions
int lengthBgExec;
int redirection;

//Creation custom behavior for ignoring choosen signals
void SIG_IGN_custom(int signum){
	printf("\nmsh> ");
	fflush(stdout);
}

//Changes the signals behavior to default one
void signalDefault(){
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
}

//Changes the signals behavior to custom
void signalIgnore(){
	signal(SIGINT, SIG_IGN_custom);
	signal(SIGQUIT, SIG_IGN_custom);

	/*signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);*/
}

//Self-programed CD function, changes current working directory depending on the input
int changeDirectory(){
	if(line->commands[0].argc > 2){
		return 1;
	}

	char cwd[BUFFER_SIZE];
	//If no argument introduced, go to HOME
	if(line->commands[0].argc == 2){
		//Diferent behavior if the direction is absolute or relative
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

//Increase or decrase the length of bgPidExec and bgCommandExec when you introduce new processes or some of then finish
void increaseJobsExecArrays(int increment){
	int i;

	lengthBgExec += increment;

	bgPidExec = (pid_t*) realloc(bgPidExec, lengthBgExec * sizeof(int));
	bgCommandExec = (char**) realloc(bgCommandExec, lengthBgExec * sizeof(char*));
	for(i=0;i<lengthBgExec;i++){
		bgCommandExec[i] = (char*) realloc(bgCommandExec[i], BUFFER_SIZE * sizeof(char));
	}
}

//Self-programed JOBS function, shows the processes executing in background
int jobs(){
	int i,j;
	int arrayDecrement;
	int status;
	pid_t currentPid;

	//If theres no background processes
	if(lengthBgExec == 1) return 0;

	arrayDecrement = 0;

	//Goes throught the list
	for(i=0;i<lengthBgExec-1;i++){
		currentPid = waitpid(bgPidExec[i], &status, WNOHANG);

		//If process finished, delete from background processes array 
		if(currentPid != 0 && bgPidExec[i] != 0){
			arrayDecrement--;
			for(j=i;j<lengthBgExec-1;j++){
				bgPidExec[j] = bgPidExec[j+1];
				strcpy(bgCommandExec[j], bgCommandExec[j+1]);
			}
			i--;

		} else if(bgPidExec[i] != 0) { //If not, prints it
			printf("[%6d] %12d - %s", i+1, bgPidExec[i], bgCommandExec[i]);	
		}
	}

	//If theres any change, adapts arrays to new size
	if(arrayDecrement != 0){
		increaseJobsExecArrays(arrayDecrement);
	}
}

//Adds new process to the jobs array when it's executing in background
void fillJobsExecArray(pid_t pid){
	bgPidExec[lengthBgExec - 1] = pid;
	strcpy(bgCommandExec[lengthBgExec - 1], buf);

	printf("[%d] %d\n", lengthBgExec, bgPidExec[lengthBgExec-1]);

	//Adapts arrays to new size
	increaseJobsExecArrays(1);	
}


//Self-programed JOBS function, bring a process executing in background to foreground, so the minishell must wait until it ends
int foreground(){
	int i,j;
	int status;
	pid_t currentPid;

	//Gets index of the process in array, for bringing it to foreground
	if(line->commands[0].argv[1] != NULL){
		i = atoi(line->commands[0].argv[1]);
	} else {
		i = 1;
	}
	i--;

	//Wait until process finish, and deletes it from arrays
	if(i < lengthBgExec){
		currentPid = waitpid(bgPidExec[i], &status, 0);
		if(currentPid != 0){
			for(j=i;j<lengthBgExec-1;j++){
				bgPidExec[j] = bgPidExec[j+1];
				strcpy(bgCommandExec[j], bgCommandExec[j+1]);
			}
		}
	}

	//Adapts arrays to new size
	increaseJobsExecArrays(-1);
}

//Changes where stdin, stdout or stderr points depending on the instruction introduced
int redirections(){
	//If input is redirected
	if (line->redirect_input != NULL) {
		printf("redirección de entrada: %s\n", line->redirect_input);

		redirection = open(line->redirect_input, O_CREAT | O_RDONLY);

		if(redirection < 0){
			return 1;
		}

		dup2(redirection, fileno(stdin));
	}
	//If output is redirected
	if (line->redirect_output != NULL) {
		printf("redirección de salida: %s\n", line->redirect_output);

		redirection = creat (line->redirect_output ,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );

		if(redirection < 0){
			return 1;
		}

		dup2(redirection, fileno(stdout));
	}
	//If error is redirected
	if (line->redirect_error != NULL) {
		printf("redirección de error: %s\n", line->redirect_error);

		redirection = creat (line->redirect_error ,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

		if(redirection < 0){
			return 1;
		}

		dup2(redirection, fileno(stderr));
	}
}

//Re-establish where stdin, stdout or stderr points by default
int endRedirections(){
	if(line->redirect_input != NULL ){
		close(redirection);
		dup2(rIn , fileno(stdin));
	}
	if(line->redirect_output != NULL ){
		close(redirection);
		dup2(rOut , fileno(stdout));	
	}
	if(line->redirect_error != NULL ){
		close(redirection);
		dup2(rErr , fileno(stderr));	
	}
}

//Executes a 1 command instruction
int simpleInstruction(){
	pid_t pid;
	pid = fork();
	if (pid < 0){
		//Fork error
	} else if(pid == 0) {
		//Son
		execvp(line->commands[0].argv[0], line->commands[0].argv);

		exit(1);
	} else {
		//Parent
		//If it's in background, stores the process PID in jobs array, if not, wait until it ends
		if(line->background){
			fillJobsExecArray(pid);
		} else {
			waitpid(pid, NULL, 0);
		}
		
	}
}

//Executes 2 or more command instructions when they are separated by pipes, changing de input and output to link all of them
int pipedInstruction(){
	int i;
	int ** pipes;
	pid_t * pids;

	//Reserves space for the ncommands introduced and create the pipes
	pipes = (int**) malloc((line->ncommands-1) * sizeof(int*));
	for(i=0;i<line->ncommands-1;i++){
		pipes[i] = (int*) malloc(2 * sizeof(int));
		pipe(pipes[i]);
	}

	//Reserves space for the array of PIDs
	pids = (pid_t*) malloc((line->ncommands) * sizeof(pid_t));
	
	for(i=0;i<line->ncommands;i++){
		signalDefault();

		pids[i] = fork();
		if (pids[i] < 0){
			//Fork error
		} else if(pids[i] == 0) {
			//Son
			//If first process, redirects only output
			if(i == 0){  
				close(pipes[i][0]);
				dup2(pipes[i][1], fileno(stdout));
			} else if(i == line->ncommands - 1){  //If last process, redirects only input
				close(pipes[i-1][1]);
				dup2(pipes[i-1][0], fileno(stdin));
			} else { //If middle process, redirects input and output
				close(pipes[i-1][1]);
				dup2(pipes[i-1][0], fileno(stdin));
				close(pipes[i][0]);
				dup2(pipes[i][1], fileno(stdout));
			}
			
			execvp(line->commands[i].argv[0], line->commands[i].argv);
			fprintf(stderr, "Error al ejecutar el comando: %s\n", strerror(errno));

			exit(1);
			
		} else {
			//Parent
			//Closes all parent output pipes
			if(!(i==(line->ncommands-1))){
				close(pipes[i][1]);
			}
		}
	}

	//If it's in background, stores the process PID in jobs array, if not, wait until all of then ends
	if(line->background){
		fillJobsExecArray(pids[0]);
	} else {
		for(i=0;i<line->ncommands;i++){
			waitpid(pids[i], NULL, 0);
		}
	}

	//Liberates the space of the array of pipes
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

	//Stores default in, out and err file descriptors, to restore after redirections
	rIn = dup(fileno(stdin));
	rOut = dup(fileno(stdout));
	rErr = dup(fileno(stderr));

	lengthBgExec = 1;

	//Reserves space for array of PIDs in background and the instructions introduced
	bgPidExec = (pid_t*) malloc(lengthBgExec * sizeof(int));
	bgCommandExec = (char**) malloc(lengthBgExec * sizeof(char*));
	for(i=0;i<lengthBgExec;i++){
		bgCommandExec[i] = (char*) malloc(BUFFER_SIZE * sizeof(char));
	}

	//Avoid quit with CTRL+C and CTRL+\
	signalIgnore();

    printf("msh> ");
	//Repeats for each instruction introduced
	while (fgets(buf, BUFFER_SIZE, stdin)) {
		
		line = tokenize(buf);
		if (line==NULL) {
			continue;
		}

		//Tests if theres any redirection
		redirections();
		
		//If the number of commands is 1, can execute c, exit, jobs and fg
		if(line->ncommands == 1){
			if(strcmp(line->commands[0].argv[0], "cd") == 0){
				changeDirectory();
			} else if(strcmp(line->commands[0].argv[0], "exit") == 0){
				//Liberates the space of the jobs arrays
				for(i=0;i<lengthBgExec;i++){
					free(bgCommandExec[i]);
				}
				free(bgCommandExec);

				exit(0);
			} else if(strcmp(line->commands[0].argv[0], "jobs") == 0){
				jobs();
			} else if(strcmp(line->commands[0].argv[0], "fg") == 0){
				foreground();
			} else {
				simpleInstruction();
			}
			
		} else { //If number of commands is bigger or equals than 2, its a piped instruction
			pipedInstruction();
		}
		
		//Re-establish stdin, stdout and stderr
		endRedirections();
		signalIgnore();
		printf("msh> ");	
	}
}