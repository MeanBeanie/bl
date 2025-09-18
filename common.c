#include "common.h"

string string_from(char* str, int len){
	string res = {0};

	res.len = len;

	res.arr = malloc(len+1);
	strncpy(res.arr, str, len);
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
