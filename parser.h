#ifndef PARSER_H
#define PARSER_H
#include "common.h"
#include "lexer.h"

struct token;
struct da_token;
NEW_DA(struct expr, expr);

enum expr_type {
	ET_NONE,
	ET_BINARY,
	ET_LITERAL,
	ET_VAR_CREATE,
};

struct expr_binary {
	struct expr* lhs;
	struct expr* rhs;
	struct token op;
};

struct expr_literal {
	int generated;
  struct token tok;
};

struct expr_var_create {
	enum token_type type;
	struct token identifier;
	struct expr* value;
};

struct expr {
	enum expr_type type;
	union {
		struct expr_binary* binary;
		struct expr_literal* literal;
		struct expr_var_create* var_create;
	} as;
};

struct da_expr parse(struct da_token tokens);

void free_exprs(struct da_expr exprs);
const char* extype2str(enum expr_type type);
#endif // PARSER_H
