#include "leptjson.h"
#include <assert.h>  /*assert()*/
#include <math.h>  /*HUGE_VAL*/
#include <stdlib.h>  /*NULL, strtod(), free()*/
#include <errno.h>  /*errno, ERANGE*/
#include <string.h> /*memcpy()*/

#define EXPECT(c, ch) do{ assert(*c->json == ch); c->json++;}while(0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif // LEPT_PARSE_STACK_INIT_SIZE
#define PUTC(c, ch) do{ *(char*)lept_context_push(c, sizeof(char)) = (ch);}while(0)


//Ϊʲôʹ�ö�ջ��Ϊ������������ֱ��ʹ��char*��Ϊʲô�����json����Ҫʹ��realloc��
typedef struct {
	const char* json;// ����ʹ��char* ���൱��˵json��һ��ָ�������ָ��һ���ַ�������˵����һ���ַ���
	char* stack;
	size_t size, top;//Ϊʲôstack������չ������top����ʹ��ָ�룿
}lept_context;


static void* lept_context_push(lept_context* c, size_t size) {
	void* ret;
	assert(size > 0);
	if (c->top + size >= c->size) {
		if (c->size == 0) c->size = LEPT_PARSE_STACK_INIT_SIZE;
		while (c->top + size >= c->size) c->size += c->size >> 1;
	}
	c->stack = (char*)realloc(c->stack, c->size);
	ret = c->stack + c->top;
	c->top += size;
	return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
	assert(c->top >= size);
	return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c) {
	const char *p = c->json; 
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		p++;
	c->json = p;
}

static int lept_parse_litral(lept_context* c, lept_value* v, char* literal, lept_type type) {
	size_t i;
	EXPECT(c, literal[0]);
	for (i = 0; literal[i+1]; i++) {
		if (c->json[i] != literal[i+1]) return LEPT_PARSE_INVALID_VALUE;
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

static int lept_parse_string(lept_context* c, lept_value* v) {
	size_t head = c->top, len;
	EXPECT(c, '\"');
	const char* p = c->json;
	while (1) {
		char ch = *p++;
		switch (ch){
		case '\"':
			len = c->top - head;
			lept_set_string(v, (const char*)lept_context_pop(c, len), len);
			v->type = LEPT_STRING;
			c->json = p;
			return LEPT_PARSE_OK;
		case '\\':
			switch (*p++) {
			case '\"': PUTC(c, '\"'); break;
			case '\\': PUTC(c, '\\'); break;
			case 'b': PUTC(c, '\b'); break;
			case 'f': PUTC(c, '\f'); break;
			case 'n"': PUTC(c, '\n'); break;
			case 'r"': PUTC(c, '\r'); break;
			case 't': PUTC(c, '\t'); break;
			default:
				c->top = head;//������Ǳ�����Ч���ַ����뻺����
				return LEPT_PARSE_INVALID_STRING_ESCAPE;
			}
		case '\0':
			c->top = head;
			return LEPT_PARSE_MISS_QUOTATION_MARK;
		default:
			if ((unsigned char)ch < 0x20) {
				c->top = head;
				return LEPT_PARSE_INVALID_STRING_CHAR;
			}
			PUTC(c, ch);
		}
	}
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json){
		case '"': return lept_parse_string(c,v);
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
	c.size = c.top = 0;
	c.stack = NULL;
	lept_init(v);
	lept_parse_whitespace(&c);

	if((ret=lept_parse_value(&c, v))== LEPT_PARSE_OK) {
		lept_parse_whitespace(&c);
		if (c.json[0] != '\0') {
			v->type = LEPT_NULL;
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	assert(c.top == 0);
	free(c.stack);
	return ret;
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}

void lept_free(lept_value* v) {//������������ã��ͷ�v��ռ���ڴ棬�����ַ��������������
	assert(v != NULL);
	if (v->type == LEPT_STRING) free(v->u.s.s);
	v->type = LEPT_NULL;//�ͷ��ڴ�󣬰�v�������ÿգ�Ŀ����Ϊ�˱����ظ��ͷ�
}

int lept_get_boolean(const lept_value* v) {
	assert(v!=NULL && (v->type==LEPT_TRUE || v->type == LEPT_FALSE));
	return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
	assert(v != NULL);
	lept_free(v);
	v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
	assert(v != NULL);
	lept_free(v);
	v->u.n = n;
	v->type = LEPT_NUMBER;
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
	//����Ķ��������ǣ��ǿ�ָ���0���ȵ��ַ������ǺϷ���
	assert(v != NULL && (s != NULL || len == 0)); //����Ĵ���о����ѿ��ö�����Ҫ��ԭ������memcpy������Ҫ��
	lept_free(v);//�����ͷ��ڴ��ԭ���ǣ�֮ǰ���ַ���ռ�Ĵ�С����һ�εĴ�С���ܲ�ͬ��Ҫ���·����ڴ�
	v->u.s.s = (char*)malloc(len + 1);
	memcpy(v->u.s.s, s, len);//memcpy���������ЩҪ���Լ����ٽ������»����ʲôЧ��
	v->u.s.s[len] = '\0';
	v->type = LEPT_STRING;
	v->u.s.len = len;
}
