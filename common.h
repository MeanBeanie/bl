#ifndef COMMON_H
#define COMMON_H
#include <string.h>
#include <stdlib.h>

typedef struct {
	char* arr;
	size_t len;
} string;

// allocates a string for you
string string_from(char* str, int len);
void string_free(string* str);
int string_cmp(string* s1, string* s2);

#define string_begin_matching if(0){}
#define string_match(str,cmp) else if(strncmp(str.arr, cmp, str.len) == 0)
#define string_default else

struct da_meta {
	size_t count;
	size_t cap;
	size_t elem_size;
};

void da_init_meta(struct da_meta* meta, size_t elem_size);

#define NEW_DA(t,name) struct da_##name { struct da_meta meta; t * arr;  }
#define da_init(a) da_init_meta(&a.meta, sizeof(*a.arr))
#define da_append(a,elem)\
	if(a.meta.count >= a.meta.cap){\
		if(a.meta.cap == 0){\
			a.meta.cap = 8;\
			a.arr = malloc(a.meta.elem_size*a.meta.cap);\
		}\
		else{\
			a.meta.cap *= 2;\
			a.arr = realloc(a.arr, a.meta.elem_size*a.meta.cap);\
		}\
	}\
	a.arr[a.meta.count] = elem;\
	a.meta.count++
#define da_iterate(a,v) for(size_t v = 0; v < a.meta.count; v++)

#endif // COMMON_H
