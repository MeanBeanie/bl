#ifndef INTERPRETER_H
#define INTERPRETER_H
#include "common.h"
#include "lexer.h"
#include "parser.h"

struct variable {
	string id;
	enum token_type type;
	union {
		float num;
		string str;
	} as;
};

NEW_DA(struct variable, var);

struct scope {
	// TODO mayhaps keep track of backtrace in scope (i.e what called a function)
	struct da_var vars;
};

struct function {
	string name;
	struct da_expr* args;
	struct da_expr* exprs;
};

NEW_DA(struct function, func);

typedef struct result {
	int failed;
	int generated;
	union {
		int boolean;
		float num;
		string str;
	} as;
} result;

void interpret(struct da_var* prev_scope_vars, struct da_expr* exprs, struct da_expr* argv, struct da_expr* arg_ids);

#endif // INTERPRETER_H
