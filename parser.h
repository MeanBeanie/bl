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
	ET_VAR_ASSIGN,
	ET_FUN_CREATE,
	ET_FUN_CALL,
	ET_BLOCK,
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

struct expr_var_assign {
	struct token identifier;
	struct expr* value;
};

struct expr_fun_create {
	struct token identifier;
	struct da_expr args;
	struct da_expr exprs;
};

struct expr_fun_call {
	struct token identifier;
	struct da_expr args;
};

struct expr {
	int line;
	enum expr_type type;
	union {
		struct expr_binary* binary;
		struct expr_literal* literal;
		struct expr_var_create* var_create;
		struct expr_var_assign* var_assign;
		struct expr_fun_create* fun_create;
		struct expr_fun_call* fun_call;
		struct da_expr block;
	} as;
};

struct da_expr parse(struct da_token tokens);

void free_exprs(struct da_expr exprs);
const char* extype2str(enum expr_type type);
void print_expr(struct expr* expr, int indent);
#endif // PARSER_H
