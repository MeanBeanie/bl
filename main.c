#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>

int main(int argc, char* argv[]){
	if(argc < 2){
		fprintf(stderr, "Expected file as first argument\n");
		return 1;
	}

	FILE* file = fopen(argv[1], "r");
	if(file == NULL){
		perror("Failed to open file");
		return 1;
	}
	fseek(file, 0, SEEK_END);
	size_t buflen = ftell(file);
	char buffer[buflen+1];
	fseek(file, 0, SEEK_SET);
	fread(buffer, sizeof(char), buflen, file);
	buffer[buflen] = '\0';
	fclose(file);
	
	struct da_token tokens = tokenize(buffer, buflen);
	struct da_expr exprs = parse(tokens);

	if(argc >= 3 && strncmp(argv[2], "debug", 5) == 0){
		da_iterate(exprs,i){
			print_expr(&exprs.arr[i], 0);
		}
	}

	interpret(NULL, &exprs, NULL, NULL);


	free_exprs(exprs);
	free_tokens(tokens);

	return 0;
}
