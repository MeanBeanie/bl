#include "common.h"

string string_from(char* str, int len){
	string res = {0};

	res.len = 0;
	res.arr = calloc(len+1, sizeof(char));

	for(int i = 0; i < len; i++){
		if(str[i] != '\\'){
			res.arr[res.len++] = str[i];
		}
		else{
			if(i+1 >= len){ break; }
			switch(str[i+1]){
				case 'n':
				{
					res.arr[res.len++] = '\n';
					break;
				}
				case 't':
				{
					res.arr[res.len++] = '\t';
					break;
				}
				case 'r':
				{
					res.arr[res.len++] = '\r';
					break;
				}
				default:
				{
					res.arr[res.len++] = '\\';
					i--;
					break;
				}
			}
			i++;
		}
	}

	res.arr[res.len] = '\0';

	return res;
}

void string_free(string* str){
	free(str->arr);
	str->arr = NULL;
}

void da_init_meta(struct da_meta* meta, size_t elem_size){
	meta->elem_size = elem_size;
	meta->cap = 0;
	meta->count = 0;
}
