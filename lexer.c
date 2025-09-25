#include "lexer.h"
#include "common.h"
#include <ctype.h>
#include <stdio.h>

#define fail(msg) fprintf(stderr, "[error] [line %d] "msg": \"%.*s\"\n", line, len+5, start); goto failure;
#define next_tok\
	tok.raw = string_from(start, len);\
	tok.line = line;\
	da_append(res,tok);\
	tok = (struct token){0};\
	tok.type = TT_NONE

#define math_char(c,cstr,ceq,typen,typeeq)\
	else if(buffer[i] == c){\
		if(i+1 < buflen && buffer[i+1] == '='){\
			tok.type = typeeq;\
			tok.raw = string_from(ceq, 2);\
		}\
		else{\
			tok.type = typen;\
			tok.raw = string_from(cstr, 1);\
		}\
		tok.line = line;\
		da_append(res,tok);\
		tok = (struct token){0};\
	}
#define match_brace(ops,cls,prefix)\
	else if(buffer[i] == ops[0]){\
		tok.type = TT_##prefix##OPEN;\
		tok.raw = string_from(ops, 1);\
		tok.line = line;\
		da_append(res,tok);\
		tok = (struct token){0};\
	}\
	else if(buffer[i] == cls[0]){\
		tok.type = TT_##prefix##CLOSE;\
		tok.raw = string_from(cls, 1);\
		tok.line = line;\
		da_append(res,tok);\
		tok = (struct token){0};\
	}\

int is_whitespace(char c){
	return c == ' ' ||
	c == '\n' ||
	c == '\t' ||
	c == '\r';
}

struct da_token tokenize(char* buffer, size_t buflen){
	struct da_token res;
	da_init(res);

	struct token tok = {0};
	char* start = buffer;
	int len = 0;
	int line = 1;

	int in_tick = 0;

	for(size_t i = 0; i < buflen; i++){
		if(buffer[i] == '\n'){ line++; }
		else if(buffer[i] == '#'){
			while(buffer[++i] != '\n'){};
			line++;
		}

		switch(tok.type){
			case TT_FLOAT:
			case TT_INT:
			{
				if(isdigit(buffer[i])){
					len++;
					break;
				}
				else if(buffer[i] == '.'){
					if(tok.type != TT_FLOAT){
						tok.type = TT_FLOAT;
						len++;
					}
					else{
						fail("Number found with 2 decimal places??");
					}
					break;
				}

				next_tok;
				i--;
				break;
			}
			case TT_IDENT:
			{
				if((isalnum(buffer[i]) || buffer[i] == '_') && buffer[i] != '\n'){
					len++;
					break;
				}

				tok.raw = string_from(start, len);

				string_begin_matching
					string_match(tok.raw,"num"){
						tok.type = TT_VAR_NUM;
					}
					string_match(tok.raw,"str"){
						tok.type = TT_VAR_STR;
					}
					string_match(tok.raw,"fun"){
						tok.type = TT_VAR_FUN;
					}
				string_default{}

				next_tok;
				i--;
				break;
			}
			case TT_STRING:
			{
				if(buffer[i] != '"' && buffer[i] != '\''){
					len++;
					break;
				}
				len--; // gets rid of the last quote

				next_tok;
				break;
			}
			case TT_NONE:
			{
				if(isdigit(buffer[i])){
					tok.type = TT_INT;
					start = buffer+i;
					len = 1;
				}
				else if(isalpha(buffer[i])){
					tok.type = TT_IDENT;
					start = buffer+i;
					len = 1;
				}
				else if(buffer[i] == '"' || buffer[i] == '\''){
					tok.type = TT_STRING;
					start = buffer+i+1;
					len = 1;
				}
				math_char('+', "+", "+=", TT_PLUS,  TT_PLEQ)
				math_char('*', "*", "*=", TT_STAR,  TT_STEQ)
				math_char('/', "/", "/=", TT_SLASH, TT_SLEQ)
				math_char('=', "=", "==", TT_EQUAL, TT_EQEQ)
				math_char('>', ">", ">=", TT_GT,    TT_GTEQ)
				math_char('<', "<", "<=", TT_LT,    TT_LTEQ)
				math_char('!', "!", "!=", TT_BANG,  TT_BAEQ)
				match_brace("(", ")", P)
				match_brace("[", "]", B)
				match_brace("{", "}", C)
				else if(buffer[i] == '-'){
					tok.line = line;
					if(i+1 < buflen){
						if(buffer[i+1] == '='){
							// MIEQ
							tok.type = TT_MIEQ;
							tok.raw = string_from("-=", 2);
							da_append(res,tok);
							tok = (struct token){0};
							break;
						}
						else if(isdigit(buffer[i+1]) && (res.meta.count > 0 && res.arr[res.meta.count-1].type != TT_IDENT)){ // TODO make this check good
							// negative number
							tok.type = TT_INT;
							start = buffer+i;
							len = 1;
							break;
						}
						// let else fall through into TT_MINUS
					}
					tok.type = TT_MINUS;
					tok.raw = string_from("-", 1);
					da_append(res,tok);
					tok = (struct token){0};
				}
				else if(buffer[i] == '`'){
					if(in_tick){
						tok.type = TT_TCLOSE;
					}
					else{
						tok.type = TT_TOPEN;
					}
					in_tick = !in_tick;
					start = buffer+i;
					len = 1;
					next_tok;
				}
				else if(buffer[i] == ';'){
					tok.type = TT_EXPR_END;
					start = buffer+i;
					len = 1;
					next_tok;
				}
				else if(buffer[i] == ','){
					tok.type = TT_ARG_SEP;
					start = buffer+i;
					len = 1;
					next_tok;
				}
				break;
			}
			default: break;
		} // switch(tok.type)
	}

failure:
	return res;
}

void free_tokens(struct da_token a){
	for(size_t i = 0; i < a.meta.count; i++){
		string_free(&a.arr[i].raw);
	}
	free(a.arr);
	a.arr = NULL;
}

const char* toktype2str(enum token_type type){
	switch(type){
		case TT_NONE: return "<NONE>";
		case TT_INT: return "<INT>";
		case TT_STRING: return "<STRING>";
		case TT_IDENT: return "<IDENT>";
		case TT_FLOAT: return "<FLOAT>";
		case TT_PLUS: return "<PLUS>";
		case TT_MINUS: return "<MINUS>";
		case TT_STAR: return "<STAR>";
		case TT_SLASH: return "<SLASH>";
		case TT_EQUAL: return "<EQUAL>";
		case TT_PLEQ: return "<PLEQ>";
		case TT_MIEQ: return "<MIEQ>";
		case TT_STEQ: return "<STEQ>";
		case TT_SLEQ: return "<SLEQ>";
		case TT_GT: return "<GT>";
		case TT_LT: return "<LT>";
		case TT_BANG: return "<BANG>";
		case TT_GTEQ: return "<GTEQ>";
		case TT_LTEQ: return "<LTEQ>";
		case TT_EQEQ: return "<EQEQ>";
		case TT_BAEQ: return "<BAEQ>";
		case TT_POPEN: return "<POPEN>";
		case TT_BOPEN: return "<BOPEN>";
		case TT_COPEN: return "<COPEN>";
		case TT_TOPEN: return "<TOPEN>";
		case TT_PCLOSE: return "<PCLOSE>";
		case TT_BCLOSE: return "<BCLOSE>";
		case TT_CCLOSE: return "<CCLOSE>";
		case TT_TCLOSE: return "<TCLOSE>";
		case TT_EXPR_END: return "<EXPR_END>";
		case TT_ARG_SEP: return "<ARG_SEP>";
		case TT_VAR_NUM: return "<VAR_NUM>";
		case TT_VAR_STR: return "<VAR_STR>";
		case TT_VAR_FUN: return "<VAR_FUN>";
	}

	return "<Invalid Token>";
}
