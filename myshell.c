#include <stdio.h>

#include "parser.h"

int main(int argc){
    char buf[1024];
	tline * line;
    int i;

    if(argc != 1){
        return 1;
    }

    printf("msh> ");
	while (fgets(buf, 1024, stdin)) {
		
		line = tokenize(buf);
		for (i=0; i<line->commands[0].argc; i++) {
				printf("  argumento %d: %s\n", i, line->commands[0].argv[i]);
			}
		printf("msh> ");	
	}
}