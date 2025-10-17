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
	|| type == TT_VAR_STR
	|| type == TT_VAR_FUN;
}

int op_precedence(enum token_type op){
	switch(op){
		case TT_LT: case TT_LTEQ:
		case TT_GT:	case TT_GTEQ:
		case TT_EQEQ: return 1;
		case TT_MINUS:
		case TT_PLUS: return 2;
		case TT_SLASH:
		case TT_STAR: return 3;
		default: return 0;
	}
}

int is_binop(enum token_type type){
	return type == TT_PLUS || type == TT_MINUS
	|| type == TT_STAR || type == TT_SLASH
	|| type == TT_GT || type == TT_GTEQ
	|| type == TT_LT || type == TT_LTEQ
	|| type == TT_EQEQ
	;
}

int is_binhs(enum token_type type){
	return type == TT_INT
	|| type == TT_FLOAT
	|| type == TT_IDENT;
}

int its_terminal(enum token_type type){
	return type == TT_EXPR_END
	|| type == TT_COPEN;
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

	int paren_count = 0;

	while((*index) < tokens.meta.count
		&& !its_terminal(tokens.arr[*index].type)
		&& tokens.arr[*index].type != TT_ARG_SEP
	){
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
			paren_count++;
		}
		else if(type == TT_PCLOSE){
			if(paren_count == 0){ break; }
			while(holding.arr[holding.meta.count-1].type != TT_POPEN){
				da_append(output,holding.arr[holding.meta.count-1]);
				holding.meta.count--;
			}
			paren_count--;

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

	while(last_op > 0){
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

	struct da_expr* expr_target = &res;
	int in_function = 0;

	size_t index = 0;
	while(index < tokens.meta.count){
		struct expr e = {0};
		if(in_function == 2 && tokens.arr[index].type == TT_CCLOSE){
			in_function = 0;
			expr_target = &res;
			index++;
			continue;
		}
		
		if(is_var_type(tokens.arr[index].type)){
			if(tokens.arr[index].type == TT_VAR_FUN){
				struct expr fun = (struct expr){
					.type = ET_FUN_CREATE,
					.as.fun_create = malloc(sizeof(struct expr_fun_create))
				};
				fun.as.fun_create->identifier = tokens.arr[index+1];
				da_init(fun.as.fun_create->exprs);
				da_init(fun.as.fun_create->args);

				fun.line = fun.as.fun_create->identifier.line;

				expr_target = &fun.as.fun_create->exprs;

				index += 2;

				if(tokens.arr[index+1].type == TT_COPEN){
					// no args
					da_append(res,fun);
					in_function = 2;
					index += 2;
				}
				else if(tokens.arr[index].type == TT_POPEN){
					index++;
					while(tokens.arr[index].type != TT_EQUAL){
						if(!is_var_type(tokens.arr[index].type)){
							index++;
							continue;
						}

						struct expr arg = (struct expr){
							.type = ET_VAR_CREATE,
							.as.var_create = malloc(sizeof(struct expr_var_create))
						};
						arg.as.var_create->type = tokens.arr[index].type;
						arg.as.var_create->identifier = tokens.arr[index+1];
						arg.as.var_create->value = NULL;
						da_append(fun.as.fun_create->args,arg);
						// lowk does just ignore ARG_SEP but c'est pas grave
						index += 3;
					}

					da_append(res,fun);

					if(tokens.arr[index+1].type == TT_COPEN){
						in_function = 2;
						index += 2;
					}
					else{
						in_function = 1;
						index += 2;
					}
				}
				else {
					da_append(res,fun);
					in_function = 1;
				};
			}
			else{
				e = (struct expr){
					.type = ET_VAR_CREATE,
					.as.var_create = malloc(sizeof(struct expr_var_create))
				};

				e.as.var_create->type = tokens.arr[index].type;
				e.as.var_create->identifier = tokens.arr[index+1];
				e.line = e.as.var_create->identifier.line;
				
				index += 3;
				if(its_terminal(tokens.arr[index-1].type)){
					e.as.var_create->value = NULL;
				}
				else{
					e.as.var_create->value = malloc(sizeof(struct expr));
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
			}
		}
		else if(tokens.arr[index].type == TT_EQUAL){
			e = (struct expr){
				.type = ET_VAR_ASSIGN,
				.as.var_assign = malloc(sizeof(struct expr_var_assign))
			};

			e.as.var_assign->identifier = tokens.arr[index-1];
			e.line = e.as.var_assign->identifier.line;

			index++;
			if(tokens.arr[index].type != TT_STRING){
					e.as.var_assign->value = malloc(sizeof(struct expr));
					struct expr b = shunting_yard(tokens, &index);
					memcpy(e.as.var_assign->value, &b, sizeof(struct expr));
					index++;
			}
			else{
					e.as.var_assign->value = malloc(sizeof(struct expr));
					struct expr lit = (struct expr){
						.type = ET_LITERAL,
						.as.literal = malloc(sizeof(struct expr_literal))
					};
					lit.as.literal->generated = 0;
					lit.as.literal->tok = tokens.arr[index];
					memcpy(e.as.var_assign->value, &lit, sizeof(struct expr));
					index += 2;
			}
		}
		else if(index+1 < tokens.meta.count
		&& tokens.arr[index].type == TT_IDENT
		&& tokens.arr[index+1].type == TT_POPEN){
			e = (struct expr){
				.type = ET_FUN_CALL,
				.as.fun_call = malloc(sizeof(struct expr_fun_call))
			};
			da_init(e.as.fun_call->args);

			e.as.fun_call->identifier = tokens.arr[index];
			e.line = e.as.fun_call->identifier.line;

			index += 2;
			while(tokens.arr[index].type != TT_PCLOSE && !its_terminal(tokens.arr[index].type)){
				if(is_binhs(tokens.arr[index].type)){
					struct expr shunt_expr = {0};
					shunt_expr = shunting_yard(tokens, &index);
					da_append(e.as.fun_call->args, shunt_expr);
				}
				else if(tokens.arr[index].type == TT_STRING){
					struct expr lit = (struct expr){
						.type = ET_LITERAL,
						.as.literal = malloc(sizeof(struct expr_literal))
					};
					lit.as.literal->generated = 0;
					lit.as.literal->tok = tokens.arr[index];
					da_append(e.as.fun_call->args, lit);
					if(tokens.arr[index+1].type == TT_ARG_SEP){
						index += 2; // skips past the arg seperator
					}
					else{
						index++;
					}
				}
				else{ index++; }
			}
		}
		else if(tokens.arr[index].type == TT_COPEN){
			struct expr block = (struct expr){
				.type = ET_BLOCK,
				.line = tokens.arr[index].line
			};
			da_init(block.as.block);

			da_append((*expr_target), block);

			expr_target = &expr_target->arr[expr_target->meta.count-1].as.block;
			index++;
			in_function = 2;
		}
		else{
			index++;
		}

		if(e.type != ET_NONE){
			da_append((*expr_target),e);
			if(in_function == 1){
				in_function = 0;
				expr_target = &res;
			}
		}
	}

	return res;
}

void free_expr(struct expr* expr){
	if(expr == NULL){ return; }
	switch(expr->type){
		case ET_NONE: break;
		case ET_BINARY:
		{ // don't free binary->op as its handled by tokens
			free_expr(expr->as.binary->lhs);
			free_expr(expr->as.binary->rhs);
			if(expr->as.binary != NULL){ free(expr->as.binary); }
			expr->as.binary = NULL;
			break;
		}
		case ET_LITERAL:
		{
			if(expr->as.literal != NULL){ free(expr->as.literal); }
			expr->as.literal = NULL;
			break;
		}
		case ET_VAR_CREATE:
		{
			free_expr(expr->as.var_create->value);
			if(expr->as.var_create != NULL){ free(expr->as.var_create); }
			expr->as.var_create = NULL;
			break;
		}
		case ET_VAR_ASSIGN:
		{
			free_expr(expr->as.var_assign->value);
			if(expr->as.var_assign != NULL){ free(expr->as.var_assign); }
			expr->as.var_assign = NULL;
			break;
		}
		case ET_FUN_CALL:
		{
			free_exprs(expr->as.fun_call->args);
			if(expr->as.fun_call != NULL){ free(expr->as.fun_call); }
			expr->as.fun_call = NULL;
			break;
		}
		case ET_FUN_CREATE:
		{
			free_exprs(expr->as.fun_create->exprs);
			free_exprs(expr->as.fun_create->args);
			if(expr->as.fun_create != NULL){ free(expr->as.fun_create); }
			expr->as.fun_create = NULL;
			break;
		}
		case ET_BLOCK:
		{
			free_exprs(expr->as.block);
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
		case ET_VAR_ASSIGN: return "<VAR_ASSIGN>";
		case ET_FUN_CALL: return "<FUN_CALL>";
		case ET_FUN_CREATE: return "<FUN_CREATE>";
		case ET_BLOCK: return "<BLOCK>";
	}

	return "<Invalid Expr>";
}

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
			if(expr->as.var_create->value == NULL){
				print_ind("<No Value Set>");
			}
			else{
				print_ind("Value:");
				print_expr(expr->as.var_create->value, indent+1);
			}
			break;
		}
		case ET_VAR_ASSIGN:
		{
			print_ind("Ident: %s", expr->as.var_assign->identifier.raw.arr);
			print_ind("Value:");
			print_expr(expr->as.var_assign->value, indent+1);
			break;
		}
		case ET_FUN_CREATE:
		{
			print_ind("Ident: %s", expr->as.fun_create->identifier.raw.arr);
			print_ind("Args:");					
			for(size_t i = 0; i < expr->as.fun_create->args.meta.count; i++){
				print_expr(&expr->as.fun_create->args.arr[i], indent+1);
			}
			print_ind("Exprs:");
			for(size_t i = 0; i < expr->as.fun_create->exprs.meta.count; i++){
				print_expr(&expr->as.fun_create->exprs.arr[i], indent+1);
			}
			break;
		}
		case ET_FUN_CALL:
		{
			print_ind("Ident: \"%s\"", expr->as.fun_call->identifier.raw.arr);
			print_ind("Args:");
			for(size_t i = 0; i < expr->as.fun_call->args.meta.count; i++){
				print_expr(&expr->as.fun_call->args.arr[i], indent+1);
			}
			break;
		}
		case ET_BLOCK:
		{
			print_ind("START_BLOCK");
			da_iterate(expr->as.block,i){
				print_expr(&expr->as.block.arr[i], indent+1);
			}
			print_ind("END_BLOCK");
			break;
		}
	}

	printf("\n");
}
