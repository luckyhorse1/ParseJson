#include "leptjson.h"
#include <assert.h>  /*assert()*/
#include <math.h>  /*HUGE_VAL*/
#include <stdlib.h>  /*NULL, strtod(), free()*/
#include <errno.h>  /*errno, ERANGE*/
#include <string.h> /*memcpy()*/

#define EXPECT(c, ch) do{ assert(*c->json == ch);}while(0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

typedef struct {
	const char* json;// 这里使用char* ，相当于说json是一个指针变量，指向一个字符。可以说他是一个字符串
}lept_context;


static void lept_parse_whitespace(lept_context* c) {
	const char *p = c->json; 
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		p++;
	c->json = p;
}

static int lept_parse_litral(lept_context* c, lept_value* v, char* literal, lept_type type) {
	size_t i;
	EXPECT(c, literal[0]);
	for (i = 0; literal[i]; i++) {
		if (c->json[i] != literal[i]) return LEPT_PARSE_INVALID_VALUE;
	}
	c->json += i;
	v->type = type;
	return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
	char* p = c->json;
	if (*p == '-') p++;
	if (*p == '0') p++;
	else {
		if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++);
	}

	if (*p == '.') {
		p++;
		if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++);
	}

	if (*p == 'e' || *p == 'E') {
		p++;
		if (*p == '-' || *p == '+') p++;
		for (p++; ISDIGIT(*p); p++);
	}

	errno = 0;
	v->u.n = strtod(c->json, NULL);
	if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL)) return LEPT_PARSE_NUMBER_TOO_BIG;
	c->json = p;
	v->type = LEPT_NUMBER;
	return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json){
		case 'n': return lept_parse_litral(c, v, "null", LEPT_NULL);
		case 't': return lept_parse_litral(c, v, "true", LEPT_TRUE);
		case 'f': return lept_parse_litral(c, v, "false", LEPT_FALSE);
		case '\0': return LEPT_PARSE_EXPECT_VALUE;
		default: return lept_parse_number(c, v);
	}
}


int lept_parse(lept_value* v, const char* json) {
	lept_context c;
	int ret;
	assert(v != NULL);
	c.json = json;
	v->type = LEPT_NULL;
	lept_parse_whitespace(&c);

	if((ret=lept_parse_value(&c, v))== LEPT_PARSE_OK) {
		lept_parse_whitespace(&c);
		if (c.json[0] != '\0') {
			v->type = LEPT_NULL;
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	return ret;
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}

void lept_free(lept_value* v) {
	assert(v != NULL);
	if (v->type == LEPT_STRING) free(v->u.s.s);
	v->type = LEPT_NULL;
}

int lept_get_boolean(const lept_value* v) {
	//TODO
	return 0;
}

void lept_set_boolean(int b) {
	//TODO
}

double lept_get_number(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
	//TODO
	/*assert(v != NULL);
	if (v->type == LEPT_NUMBER) {
		v->u.n = n;
	}*/
}

const char* lept_get_string(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_STRING);
	return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_STRING);
	return v->u.s.len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
	assert(v != NULL && (s != NULL || len == 0)); //这里的代码感觉很难看得懂，重要的原因在于memcpy函数的要求
	lept_free(v);
	v->u.s.s = (char*)malloc(len + 1);
	memcpy(v->u.s.s, s, len);//memcpy的入参有哪些要求，以及在临界条件下会出现什么效果
	v->u.s.s[len] = '\0';
	v->type == LEPT_STRING;
	v->u.s.len = len;
}
