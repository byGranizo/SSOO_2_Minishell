#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include "parser.h"

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
		waitpid(pid, NULL, 0);
	}
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

    printf("msh> ");
	while (fgets(buf, 1024, stdin)) {
		
		line = tokenize(buf);
		if (line==NULL) {
			continue;
		}
		if (line->redirect_input != NULL) {
			
		}
		if (line->redirect_output != NULL) {
			
		}
		if (line->redirect_error != NULL) {
			
		}
		if (line->background) {
			
		}
		
		printf("msh> ");	
	}
}