#include "lexer.h"
#include "parser.h"
#include <stdio.h>

#define print_ind(fmt,...) printf("%s"fmt"\n", tab, ##__VA_ARGS__)
void print_expr(struct expr* expr, int indent){
	char tab[indent+1];
	for(int i = 0; i < indent; i++){
		tab[i] = '\t';
	}
	tab[indent] = '\0';
	print_ind("EXPR [%s]", extype2str(expr->type));

	switch(expr->type){
		case ET_NONE: break;
		case ET_BINARY:
		{
			print_ind("LHS:");
			print_expr(expr->as.binary->lhs, indent+1);
			print_ind("OP:");
			print_ind("%s\n", toktype2str(expr->as.binary->op.type));
			print_ind("RHS:");
			print_expr(expr->as.binary->rhs, indent+1);
			break;
		}
		case ET_LITERAL:
		{
			print_ind("Value: %s", toktype2str(expr->as.literal->tok.type));
			print_ind("Raw: %s", expr->as.literal->tok.raw.arr);
			break;
		}
		case ET_VAR_CREATE:
		{
			print_ind("Type: %s", toktype2str(expr->as.var_create->type));
			print_ind("Ident: %s", expr->as.var_create->identifier.raw.arr);
			print_ind("Value:");
			print_expr(expr->as.var_create->value, indent+1);
			break;
		}
	}

	printf("\n");
}

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

	da_iterate(exprs,i){
		print_expr(&exprs.arr[i], 0);
	}

	free_tokens(tokens);
	free_exprs(exprs);

	return 0;
}
