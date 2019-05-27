#include "leptjson.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#define EXPECT(c, ch) do{ assert(*c->json == ch); c->json++;}while(0)

typedef struct {
	const char* json;// 这里使用char* ，相当于说json是一个指针变量，指向一个字符。可以说他是一个字符串
}lept_context;


static void lept_parse_whitespace(lept_context* c) {
	const char *p = c->json; 
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		p++;
	c->json = p;
}

static int lept_parse_null(lept_context* c, lept_value* v) {
	EXPECT(c, 'n');// 首先判断顶第一个字母对不对
	if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')//接下来判定后面的每一个字符对不对
		return LEPT_PARSE_INVALID_VALUE;
	c->json += 3;// 这句话没看懂
	v->type = LEPT_NULL;
	return LEPT_PARSE_OK;
}

static int lept_parse_true(lept_context* c, lept_value* v) {
	EXPECT(c, 't');
	if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
		return LEPT_PARSE_INVALID_VALUE;
	c->json += 3;
	v->type = LEPT_TRUE;
	return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context* c, lept_value* v) {
	EXPECT(c, 'f');
	if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
		return LEPT_PARSE_INVALID_VALUE;
	c->json += 4;
	v->type = LEPT_FALSE;
	return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
	char* end;
	v->n = strtod(c->json, &end);
	if (c->json == end) {
		return LEPT_PARSE_INVALID_VALUE;
	}
	c->json = end;
	v->type = LEPT_NUMBER;
	return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json){
		case 'n': return lept_parse_null(c, v);
		case 't': return lept_parse_true(c, v);
		case 'f': return lept_parse_false(c, v);
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

double lept_get_number(const lept_value* v) {
	assert(v != NULL);
	return v->n;
}