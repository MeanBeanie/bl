#include "parser.h"
#include "lexer.h"
#include <stdio.h>

#define down_shift_two\
	for(size_t i = working_op; i < output.meta.count; i++){\
		output.arr[i-2] = output.arr[i];\
	}\
	char index_str[2] = {\
		hold_index-1+'0',\
		'\0'\
	};\
	output.arr[working_op-2] = (struct token){\
		.line = -1,\
		.type = TT_NONE,\
		.raw = string_from(index_str, 1)\
	};\
	output.meta.count -= 2;\
	last_op -= 2

int is_var_type(enum token_type type){
	return type == TT_VAR_NUM
	|| type == TT_VAR_STR;
}

int op_precedence(enum token_type op){
	switch(op){
		case TT_MINUS:
		case TT_PLUS: return 1;
		case TT_SLASH:
		case TT_STAR: return 2;
		default: return 0;
	}
}

int is_binop(enum token_type type){
	return type == TT_PLUS
	|| type == TT_MINUS
	|| type == TT_STAR
	|| type == TT_SLASH;
}

int is_binhs(enum token_type type){
	return type == TT_INT
	|| type == TT_FLOAT
	|| type == TT_IDENT;
}

struct expr make_binary(struct token lhs, struct token op, struct token rhs){
	struct expr e = (struct expr){
		.type = ET_BINARY,
		.as.binary = malloc(sizeof(struct expr_binary))
	};

	e.as.binary->op = op;

	e.as.binary->lhs = malloc(sizeof(struct expr));
	e.as.binary->lhs->type = ET_LITERAL;
	e.as.binary->lhs->as.literal = malloc(sizeof(struct expr_literal));
	e.as.binary->lhs->as.literal->generated = 0;
	e.as.binary->lhs->as.literal->tok = lhs;
	
	e.as.binary->rhs = malloc(sizeof(struct expr));
	e.as.binary->rhs->type = ET_LITERAL;
	e.as.binary->rhs->as.literal = malloc(sizeof(struct expr_literal));
	e.as.binary->rhs->as.literal->generated = 0;
	e.as.binary->rhs->as.literal->tok = rhs;

	return e;
}

struct expr shunting_yard(struct da_token tokens, size_t* index){
	struct da_token output;
	da_init(output);
	struct da_token holding;
	da_init(holding);

	while((*index) < tokens.meta.count && tokens.arr[*index].type != TT_EXPR_END){
		enum token_type type = tokens.arr[*index].type;
		if(is_binop(type)){
			if(holding.meta.count != 0){
				enum token_type top_type = holding.arr[holding.meta.count-1].type;
				while(op_precedence(type) < op_precedence(top_type) && top_type != TT_POPEN){
					da_append(output,holding.arr[holding.meta.count-1]);
					holding.meta.count--;
					if(holding.meta.count == 0){
						break;
					}
					top_type = holding.arr[holding.meta.count-1].type;
				}
			}
			da_append(holding,tokens.arr[*index]);
		}
		else if(is_binhs(type)){
			da_append(output,tokens.arr[*index]);
		}
		else if(type == TT_POPEN){
			da_append(holding,tokens.arr[*index]);
		}
		else if(type == TT_PCLOSE){
			while(holding.arr[holding.meta.count-1].type != TT_POPEN){
				da_append(output,holding.arr[holding.meta.count-1]);
				holding.meta.count--;
			}

			holding.meta.count--; // get rid of the popen
		}
		(*index)++;
	} // while < meta count & ! expr end

	while(holding.meta.count > 0){
		da_append(output,holding.arr[holding.meta.count-1]);
		holding.meta.count--;
	}

	if(output.meta.count == 1){
		struct expr e = (struct expr){
			.type = ET_LITERAL,
			.as.literal = malloc(sizeof(struct expr_literal))
		};
		e.as.literal->generated = 0;
		e.as.literal->tok = output.arr[0];
		return e;
	}

	int last_op = -1;
	for(size_t i = output.meta.count-1; i > 1; i--){
		if(is_binop(output.arr[i].type)){ last_op = i; break; }
	}

	if(is_binhs(output.arr[last_op-1].type)
	&& is_binhs(output.arr[last_op-2].type)){
		// simple expression
		return make_binary(output.arr[last_op-2], output.arr[last_op], output.arr[last_op-1]);
	}

	struct expr expr_hold[10] = {0};
	int hold_index = 0;

	while(last_op != 0){
		int working_op = last_op;
		while(is_binop(output.arr[working_op-1].type)){
			working_op--;
		}
		while(is_binop(output.arr[working_op-2].type)){
			working_op -= 2;
		}

		if(is_binhs(output.arr[working_op-1].type)
		&& is_binhs(output.arr[working_op-2].type)){
			expr_hold[hold_index++] = make_binary(output.arr[working_op-2], output.arr[working_op], output.arr[working_op-1]);
			if(hold_index > 9){ hold_index = 0; }
			down_shift_two;
		}
		else if(is_binhs(output.arr[working_op-1].type)
		&& output.arr[working_op-2].type == TT_NONE){
			// lhs is expr
			struct expr expr = (struct expr){
				.type = ET_BINARY,
				.as.binary = malloc(sizeof(struct expr_binary))
			};

			expr.as.binary->op = output.arr[working_op];

			int lhs_index = atoi(output.arr[working_op-2].raw.arr);
			expr.as.binary->lhs = malloc(sizeof(struct expr));
			memcpy(expr.as.binary->lhs, &expr_hold[lhs_index], sizeof(struct expr));

			expr.as.binary->rhs = malloc(sizeof(struct expr));
			expr.as.binary->rhs->type = ET_LITERAL;
			expr.as.binary->rhs->as.literal = malloc(sizeof(struct expr_literal));
			expr.as.binary->rhs->as.literal->generated = 0;
			expr.as.binary->rhs->as.literal->tok = output.arr[working_op-1];

			expr_hold[hold_index++] = expr;
			if(hold_index > 9){ hold_index = 0; }
			down_shift_two;
		}
		else if(is_binhs(output.arr[working_op-2].type)
		&& output.arr[working_op-1].type == TT_NONE){
			// rhs is expr
			struct expr expr = (struct expr){
				.type = ET_BINARY,
				.as.binary = malloc(sizeof(struct expr_binary))
			};

			expr.as.binary->op = output.arr[working_op];

			int rhs_index = atoi(output.arr[working_op-1].raw.arr);
			expr.as.binary->rhs = malloc(sizeof(struct expr));
			memcpy(expr.as.binary->rhs, &expr_hold[rhs_index], sizeof(struct expr));

			expr.as.binary->lhs = malloc(sizeof(struct expr));
			expr.as.binary->lhs->type = ET_LITERAL;
			expr.as.binary->lhs->as.literal = malloc(sizeof(struct expr_literal));
			expr.as.binary->lhs->as.literal->generated = 0;
			expr.as.binary->lhs->as.literal->tok = output.arr[working_op-2];
			
			expr_hold[hold_index++] = expr;
			if(hold_index > 9){ hold_index = 0; }
			down_shift_two;
		}
		else if(output.arr[working_op-1].type == TT_NONE
		&& output.arr[working_op-2].type == TT_NONE){
			// both are expr
			struct expr expr = (struct expr){
				.type = ET_BINARY,
				.as.binary = malloc(sizeof(struct expr_binary))
			};

			expr.as.binary->op = output.arr[working_op];

			int rhs_index = atoi(output.arr[working_op-1].raw.arr);
			expr.as.binary->rhs = malloc(sizeof(struct expr));
			memcpy(expr.as.binary->rhs, &expr_hold[rhs_index], sizeof(struct expr));

			int lhs_index = atoi(output.arr[working_op-2].raw.arr);
			expr.as.binary->lhs = malloc(sizeof(struct expr));
			memcpy(expr.as.binary->lhs, &expr_hold[lhs_index], sizeof(struct expr));
			
			expr_hold[hold_index++] = expr;
			if(hold_index > 9){ hold_index = 0; }
			down_shift_two;
		}
	}

	// DO NOT use free_tokens since the strings are in another castle
	free(output.arr);
	free(holding.arr);

	(*index)++;

	return expr_hold[hold_index-1];
}

struct da_expr parse(struct da_token tokens){
	struct da_expr res = {0};
	da_init(res);

	size_t index = 0;
	while(index < tokens.meta.count){
		struct expr e = {0};
		
		if(is_var_type(tokens.arr[index].type)){
			e = (struct expr){
				.type = ET_VAR_CREATE,
				.as.var_create = malloc(sizeof(struct expr_var_create))
			};

			e.as.var_create->type = tokens.arr[index].type;
			e.as.var_create->identifier = tokens.arr[index+1];
			index += 3;
			switch(e.as.var_create->type){
				case TT_VAR_NUM:
				{
					e.as.var_create->value = malloc(sizeof(struct expr));
					struct expr b = shunting_yard(tokens, &index);
					memcpy(e.as.var_create->value, &b, sizeof(struct expr));
					break;
				}
				case TT_VAR_STR:
				{
					e.as.var_create->value = malloc(sizeof(struct expr));
					struct expr lit = (struct expr){
						.type = ET_LITERAL,
						.as.literal = malloc(sizeof(struct expr_literal))
					};
					lit.as.literal->generated = 0;
					lit.as.literal->tok = tokens.arr[index];
					memcpy(e.as.var_create->value, &lit, sizeof(struct expr));
					index += 2;
					break;
				}
				default: break;
			}
		}

		if(e.type != ET_NONE){
			da_append(res,e);
		}
	}

	return res;
}

void free_expr(struct expr* expr){
	switch(expr->type){
		case ET_NONE: break;
		case ET_BINARY:
		{ // don't free binary->op as its handled by tokens
			free_expr(expr->as.binary->lhs);
			free_expr(expr->as.binary->rhs);
			free(expr->as.binary);
			break;
		}
		case ET_LITERAL:
		{
			free(expr->as.literal);
			break;
		}
		case ET_VAR_CREATE:
		{
			free_expr(expr->as.var_create->value);
			free(expr->as.var_create);
			break;
		}
	}
}
void free_exprs(struct da_expr exprs){
	da_iterate(exprs,i){
		free_expr(&exprs.arr[i]);
	}
}

const char* extype2str(enum expr_type type){
	switch(type){
		case ET_NONE: return "<NONE>";
		case ET_BINARY: return "<BINARY>";
		case ET_LITERAL: return "<LITERAL>";
		case ET_VAR_CREATE: return "<VAR_CREATE>";
	}
	return "<Invalid Expr>";
}
