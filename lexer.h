#ifndef LEXER_H
#define LEXER_H
#include "common.h"

enum token_type {
	TT_NONE,
	
	/* NOTE: no TT_CHAR as it will be the same as 
		 a single character string */
	TT_INT,   TT_STRING, TT_IDENT,
	TT_FLOAT,

	TT_PLUS, TT_MINUS, TT_STAR, TT_SLASH, TT_EQUAL,
	TT_PLEQ, TT_MIEQ,  TT_STEQ, TT_SLEQ, /* UNUSED */

	TT_GT,   TT_LT, /*TT_EQ */ TT_BANG,
	TT_GTEQ, TT_LTEQ, TT_EQEQ, TT_BAEQ,

	/*
		 P - ()
		 > Used to group expressions
		 B - []
		 > Used to index arrays or strings
		 C - {}
		 > Used to block multiple expressions into one, for functions
		 T - ``
		 > Gets replaced with a string containing the stdout/stderr
		 > of the command that is in the ticks at runtime
	*/
	TT_POPEN,  TT_BOPEN,  TT_COPEN,  TT_TOPEN,
	TT_PCLOSE, TT_BCLOSE, TT_CCLOSE, TT_TCLOSE,

	TT_EXPR_END, TT_ARG_SEP,

	TT_VAR_NUM, TT_VAR_STR, TT_VAR_FUN,
};

struct token {
	int line; // for error reporting
	enum token_type type;
	string raw;
};

NEW_DA(struct token, token);

struct da_token tokenize(char* buffer, size_t buflen);

void free_tokens(struct da_token a);

const char* toktype2str(enum token_type type);

#endif // LEXER_H
