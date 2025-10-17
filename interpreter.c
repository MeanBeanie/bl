#include "interpreter.h"
#include "parser.h"
#include "lexer.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#define error(fmt,...) log_error("\n[ERROR]", fmt, ##__VA_ARGS__)
#define fatal(fmt,...) log_error("\n[FATAL]", fmt, ##__VA_ARGS__)
void log_error(char* label, char* fmt, ...){
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "%s", label);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

result simplify_to_float(struct scope* scope, struct expr* expr);
result simplify_to_string(struct scope* scope, struct expr* expr);

// returns 0 on fail
int load_var(struct variable* var, struct scope* scope, struct expr* value);
struct variable* find_var(struct scope* scope, string identifier);

// returns 0 on fail
int generate_args(struct scope* scope, struct da_var* vars, struct da_expr* args, struct da_expr* arg_ids);

void interpret(struct da_var* prev_scope_vars, struct da_expr* exprs, struct da_expr* argv, struct da_expr* arg_ids){
	struct scope scope = {0};
	da_init(scope.vars);
	
	struct da_func funcs;
	da_init(funcs);

	if(argv != NULL || arg_ids != NULL){
		if(argv->meta.count != arg_ids->meta.count){
			fatal("Failed to call function with improper amout of args (%zu given, %zu expected)", argv->meta.count, arg_ids->meta.count);
			goto end_interpret;
		}

		if(prev_scope_vars != NULL){
			struct scope prev_scope = (struct scope){
				.vars = (*prev_scope_vars)
			};

			if(!generate_args(&prev_scope, &scope.vars, argv, arg_ids)){
				goto end_interpret;
				return;
			}
		}
		else{
			if(!generate_args(NULL, &scope.vars, argv, arg_ids)){
				goto end_interpret;
				return;
			}
		}
	}
	else if(prev_scope_vars != NULL){
		// subscope, we want access to the previous scopes variables
		da_iterate((*prev_scope_vars),i){
			da_append(scope.vars, prev_scope_vars->arr[i]);
		}
	}

	int old_skip_hold = 0;
	int skip_hold = 0;

	da_iterate((*exprs),i){
		switch(exprs->arr[i].type){
			case ET_VAR_CREATE:
			{
				struct variable var = (struct variable){
					.type = exprs->arr[i].as.var_create->type,
				};

				if(!load_var(&var, &scope, exprs->arr[i].as.var_create->value)){
					i = exprs->meta.count+1;
					break;
				}

				var.id = exprs->arr[i].as.var_create->identifier.raw;

				da_append(scope.vars,var);
				break;
			}
			case ET_VAR_ASSIGN:
			{
				struct variable* var = find_var(&scope, exprs->arr[i].as.var_assign->identifier.raw);
				if(var == NULL){
					fatal("[line %d] failed to find variable %s\n", exprs->arr[i].line, exprs->arr[i].as.var_assign->identifier.raw.arr);
					i = exprs->meta.count+1;
					break;
				}

				if(!load_var(var, &scope, exprs->arr[i].as.var_assign->value)){
					i = exprs->meta.count+1;
				}
				break;
			}
			case ET_FUN_CREATE:
			{
				struct function f = (struct function){
					.name = exprs->arr[i].as.fun_create->identifier.raw,
					.exprs = &exprs->arr[i].as.fun_create->exprs,
					.args = &exprs->arr[i].as.fun_create->args,
				};

				da_append(funcs,f);
				break;
			}
			case ET_FUN_CALL:
			{
				struct expr_fun_call* fun_call = exprs->arr[i].as.fun_call;
				string_begin_matching
					string_match(exprs->arr[i].as.fun_call->identifier.raw, "for"){
						// NOTE: UNUSED CURRENTLY
						if(scope.loop_index == 0 || (scope.loop_index != 0 && scope.loop_stack[scope.loop_index-1] != i)){
							scope.loop_stack[scope.loop_index++] = i;
						}
						if(fun_call->args.arr[0].type != ET_LITERAL || fun_call->args.arr[1].type != ET_LITERAL){
							fatal("[line %d] for expects literals in the first two args, not: %s and %s\n", exprs->arr[i].line, extype2str(fun_call->args.arr[0].type), extype2str(fun_call->args.arr[1].type));
							i = exprs->meta.count+1;
							goto et_fun_call_end;
						}
						// if the variable doesn't exist, this is our first iteration and we should create it
						struct variable* iterator = find_var(&scope, fun_call->args.arr[0].as.literal->tok.raw);
						if(iterator == NULL){
							struct variable v = {0};
							v.type = TT_VAR_NUM;
							result val_res = simplify_to_float(&scope, &fun_call->args.arr[1]);
							if(val_res.failed){
								fatal("[line %d] could not assign iterator to value", exprs->arr[i].line);
								i = exprs->meta.count+1;
								goto et_fun_call_end;
							}
							v.as.num = val_res.as.num;
							v.id = fun_call->args.arr[0].as.literal->tok.raw;
							
							da_append(scope.vars,v);
						}
						else{
							result new_iterator_val = simplify_to_float(&scope, &fun_call->args.arr[3]);
							if(new_iterator_val.failed){
								fatal("[line %d] cannot modify iterator with %s\n", exprs->arr[i].line, extype2str(fun_call->args.arr[3].type));
								i = exprs->meta.count+1;
								goto et_fun_call_end;
							}

							iterator->as.num = new_iterator_val.as.num;
						}

						result cond_res = simplify_to_float(&scope, &fun_call->args.arr[2]);
						if(cond_res.failed){
							fatal("[line %d] cannot for %s\n", exprs->arr[i].line, extype2str(fun_call->args.arr[2].type));
							i = exprs->meta.count+1;
							goto et_fun_call_end;
						}

						if(cond_res.as.num == 0.0f){
							i++; // skip next expression, loop failed
							scope.loop_index--;
						}
						else{
							// NOTE using skip_hold DOES NOT allow for nested loops
							skip_hold = -2;
						}
						
						break;
					}
					string_match(fun_call->identifier.raw, "while"){
						// NOTE: UNUSED CURRENTLY
						if(scope.loop_index == 0 || (scope.loop_index != 0 && scope.loop_stack[scope.loop_index-1] != i)){
							scope.loop_stack[scope.loop_index++] = i;
						}
						result cond_res = simplify_to_float(&scope, &fun_call->args.arr[0]);
						if(cond_res.failed){
							fatal("[line %d] cannot while %s\n", exprs->arr[i].line, extype2str(fun_call->args.arr[0].type));
							i = exprs->meta.count+1;
							goto et_fun_call_end;
						}

						if(cond_res.as.num == 0.0f){
							i++; // skip the next expression since the while loop failed
							scope.loop_index--;
						}
						else {
							// NOTE using skip_hold DOES NOT allow for nested loops
							skip_hold = -2; // skip back up to the while after running the next expression
						}

						break;
					}
					string_match(fun_call->identifier.raw,"if"){
						if(fun_call->args.meta.count != 1){
							fatal("[line %d] if expects one argument\n");
							i = exprs->meta.count+1;
							goto et_fun_call_end;
						}

						result cond_res = simplify_to_float(&scope, &fun_call->args.arr[0]);
						if(cond_res.failed){
							fatal("[line %d] cannot if %s\n", exprs->arr[i].line, extype2str(fun_call->args.arr[0].type));
							i = exprs->meta.count+1;
							goto et_fun_call_end;
						}

						int else_cond = i + 2 < exprs->meta.count
						                && exprs->arr[i+2].type == ET_FUN_CALL
						                && strncmp(exprs->arr[i+2].as.fun_call->identifier.raw.arr, "else", 4) == 0;

						if(cond_res.as.num == 0.0f){
							if(else_cond){
								i += 2; // skip to whatever else wants to do
							}
							else{
								i++; // skip next expr on false
							}
						}
						else if(else_cond){
							// we have an else we need to skip
							skip_hold = 2;
						}
						break;
					}
					string_match(fun_call->identifier.raw,"serr"){
						da_iterate(fun_call->args,j){
							result arg_res = simplify_to_string(&scope, &fun_call->args.arr[j]);
							if(arg_res.failed){
								fatal("[line %d] cannot serr %s\n", exprs->arr[i].line, extype2str(fun_call->args.arr[j].type));
								i = exprs->meta.count+1;
								goto et_fun_call_end;
							}
							fprintf(stderr, "%s", arg_res.as.str.arr);
							fflush(stderr);
							if(arg_res.generated){
								string_free(&arg_res.as.str);
							}
						}
						break;
					}
					string_match(fun_call->identifier.raw,"sout"){
						da_iterate(fun_call->args,j){
							result arg_res = simplify_to_string(&scope, &fun_call->args.arr[j]);
							if(arg_res.failed){
								fatal("[line %d] cannot sout %s\n", exprs->arr[i].line, extype2str(fun_call->args.arr[j].type));
								i = exprs->meta.count+1;
								goto et_fun_call_end;
							}
							printf("%s", arg_res.as.str.arr);
							fflush(stdout);
							if(arg_res.generated){
								string_free(&arg_res.as.str);
							}
						}
						break;
					}
				string_default{}

				da_iterate(funcs,j){
					if(strncmp(funcs.arr[j].name.arr, fun_call->identifier.raw.arr, fun_call->identifier.raw.len) == 0){
						interpret(&scope.vars, funcs.arr[j].exprs, &fun_call->args, funcs.arr[j].args);

						goto et_fun_call_end;
					}
				}

				fatal("[line %d] unknown function %s\n", exprs->arr[i].line, fun_call->identifier.raw.arr);
				i = exprs->meta.count+1;

et_fun_call_end:
				break;
			}
			case ET_BLOCK:
			{
				interpret(&scope.vars, &exprs->arr[i].as.block, NULL, NULL);
				break;
			}
			default: break;
		}

		if(skip_hold != 0 && old_skip_hold == skip_hold){
			i += skip_hold;
			skip_hold = 0;
		}
		old_skip_hold = skip_hold;
	}

end_interpret:
	if(argv == NULL && arg_ids == NULL && prev_scope_vars != NULL){
		// subscope, we want to update the previous scopes variables
		da_iterate(scope.vars,i){
			da_iterate((*prev_scope_vars),j){
				if(string_cmp(&prev_scope_vars->arr[j].id, &scope.vars.arr[i].id)){
					if(prev_scope_vars->arr[j].type == TT_VAR_NUM){ // b/c strings are already pointer based so they don't need to be updated
						prev_scope_vars->arr[j].as.num = scope.vars.arr[i].as.num;
					}
				}
			}
		}
	}

	da_iterate(scope.vars,i){
		if(scope.vars.arr[i].type == TT_VAR_STR && scope.vars.arr[i].generated){
			string_free(&scope.vars.arr[i].as.str);
		}
	}
	free(scope.vars.arr);
}

string ftos(float f){
	string res = {0};
	
	char buf[128] = {0};
	if((int)f == f){ // can be simplified to int?
		sprintf(buf, "%d", (int)f);
	}
	else{
		sprintf(buf, "%.4f", f);
	}

	res = string_from(buf, strlen(buf));

	return res;
}

float stof(char* str){
	float res = 0.f;
	int dec = 0;

	do {
		char c = (*str);
		if(c == '-'){ res *= -1; }
		else if(c == '.'){ dec = 10; }
		else if(isdigit(c)){
			if(dec != 0){
				float f = (float)(c - '0') / dec;
				res += f;
				dec *= 10;
			}
			else{
				res *= 10;
				res += (c - '0');
			}
		}
	} while(*str++);

	return res;
}

int load_var(struct variable* var, struct scope* scope, struct expr* value){
	if(var->type == TT_VAR_NUM){
		result res = simplify_to_float(scope, value);
		if(res.failed){
			return 0;
		}
		var->as.num = res.as.num;
	}
	else{ // TT_VAR_STR
		var->generated = 0;
		var->as.str = value->as.literal->tok.raw;
	}

	return 1;
}

struct variable* find_var(struct scope* scope, string identifier){
	da_iterate(scope->vars,i){ // TODO hashmaps or something, i dunno
		if(scope->vars.arr[i].id.arr == NULL || identifier.arr == NULL){ break; }
		if(strncmp(scope->vars.arr[i].id.arr, identifier.arr, identifier.len) == 0){
			return &scope->vars.arr[i];
		}
	}

	return NULL;
}

result simplify_to_float(struct scope* scope, struct expr* expr){
	result res = {0};

	switch(expr->type){
		default:
		{
			fatal("[line %d] Tried to simplify %s to float\n", expr->line, extype2str(expr->type));
			res.failed = 1;
			break;
		}
		case ET_BINARY:
		{
			float lhs,rhs;
			struct result lhs_res = simplify_to_float(scope, expr->as.binary->lhs);
			if(lhs_res.failed){
				res.failed = 1;
				break;
			}
			lhs = lhs_res.as.num;
			struct result rhs_res = simplify_to_float(scope, expr->as.binary->rhs);
			if(rhs_res.failed){
				res.failed = 1;
				break;
			}
			rhs = rhs_res.as.num;

			switch(expr->as.binary->op.type){
				case TT_LT: res.as.num = lhs < rhs; break;
				case TT_LTEQ: res.as.num = lhs <= rhs; break;
				case TT_GT: res.as.num = lhs > rhs; break;
				case TT_GTEQ: res.as.num = lhs >= rhs; break;
				case TT_EQEQ: res.as.num = lhs == rhs; break;
				case TT_MINUS: res.as.num = lhs-rhs; break;
				case TT_STAR: res.as.num = lhs*rhs; break;
				case TT_SLASH: res.as.num = lhs/rhs; break;
				case TT_PLUS:
				default: res.as.num = lhs+rhs; break;
			}
			break;
		}
		case ET_LITERAL:
		{
			if(expr->as.literal->tok.type == TT_INT
			|| expr->as.literal->tok.type == TT_FLOAT){
				res.as.num = stof(expr->as.literal->tok.raw.arr);
			}
			else if(expr->as.literal->tok.type == TT_IDENT){
				if(scope == NULL){
					fatal("[line %d] cannot simplify float to string if no scope exists\n", expr->line);
					res.failed = 1;
					break;
				}
				struct variable* var = find_var(scope, expr->as.literal->tok.raw);
				if(var == NULL){
					fatal("[line %d] Failed to find variable %s\n", expr->as.literal->tok.raw.arr);
					res.failed = 1;
					break;
				}
				
				res.as.num = var->as.num;
			}
			else {
				fatal("[line %d] Invalid literal %s simplified to float\n", expr->line, toktype2str(expr->as.literal->tok.type));
				res.failed = 1;
			}
			break;
		}
	}

	return res;
}

result simplify_to_string(struct scope* scope, struct expr* expr){
	result res = {0};

	switch(expr->type){
		default:
		{
			fatal("[line %d] tried to simplify %s to string\n", expr->line, extype2str(expr->type));
			res.failed = 1;
			break;
		}
		case ET_BINARY:
		{
			result bin_res = simplify_to_float(scope, expr);
			res.generated = 1;
			if(bin_res.failed){
				error("[line %d] failed to simplify binary expression\n", expr->line);
				res.as.str = string_from("0", 1);
				break;
			}

			res.as.str = ftos(bin_res.as.num);
			break;
		}
		case ET_LITERAL:
		{
			if(expr->as.literal->tok.type == TT_IDENT){
				if(scope == NULL){
					fatal("[line %d] cannot simplify identifier to string if no scope exists\n", expr->line);
					res.failed = 1;
					break;
				}

				struct variable* var = find_var(scope, expr->as.literal->tok.raw);
				if(var == NULL){
					fatal("[line %d] failed to find var %s\n", expr->line, expr->as.literal->tok.raw);
					res.failed = 1;
					break;
				}

				if(var->type == TT_VAR_NUM){
					res.generated = 1;
					res.as.str = ftos(var->as.num);
				}
				else{ // TT_VAR_STR
					res.as.str = var->as.str;
				}
			}
			else{
				res.as.str = expr->as.literal->tok.raw;
			}
			break;
		}
	}

	return res;
}

int generate_args(struct scope* scope, struct da_var* vars, struct da_expr* args, struct da_expr* arg_ids){
	int arg_index = 0;
	da_iterate((*args),i){
		struct variable v = (struct variable){
			.generated = 0,
			.id = arg_ids->arr[arg_index++].as.var_create->identifier.raw,
			.type = arg_ids->arr[i].as.var_create->type
		};

		if(v.type == TT_VAR_NUM){
			result v_res = simplify_to_float(scope, &args->arr[i]);
			if(v_res.failed){
				fatal("[line %d] failed to create arg %s\n", args->arr[i].line, args->arr[i].as.var_create->identifier.raw.arr);
				return 0;
			}

			v.as.num = v_res.as.num;
		}
		else if(v.type == TT_VAR_STR){
			result v_res = simplify_to_string(scope, &args->arr[i]);
			if(v_res.failed){
				fatal("[line %d] failed to create arg %s\n", args->arr[i].line, args->arr[i].as.var_create->identifier.raw.arr);
				return 0;
			}
			
			v.generated = v_res.generated;
			v.as.str = v_res.as.str;
		}

		da_append((*vars),v);
	}

	return 1;
}
