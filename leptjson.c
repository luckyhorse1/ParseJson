#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>  /*NULL, strtod(), free()*/
#include <crtdbg.h>
#include "leptjson.h"
#include <assert.h>  /*assert()*/
#include <math.h>  /*HUGE_VAL*/
#include <errno.h>  /*errno, ERANGE*/
#include <string.h> /*memcpy()*/

#define EXPECT(c, ch) do{ assert(*c->json == ch); c->json++;}while(0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif // LEPT_PARSE_STACK_INIT_SIZE
#define PUTC(c, ch) do{ *(char*)lept_context_push(c, sizeof(char)) = (ch);}while(0)


//为什么使用堆栈作为缓冲区，不能直接使用char*吗？为什么下面的json不需要使用realloc？
typedef struct {
	const char* json;// 这里使用char* ，相当于说json是一个指针变量，指向一个字符。可以说他是一个字符串
	char* stack;
	size_t size, top;//为什么stack可以扩展，所以top不能使用指针？
}lept_context;


static void* lept_context_push(lept_context* c, size_t size) {//这个函数的作用是开辟空间
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

#define STRING_ERROR(ret) do{c->top = head; return ret;}while(0)

static const char* lept_parse_hex4(const char* p, unsigned* u) {//将字符解析为码点
	int i;
	*u = 0;
	for (i = 0; i < 4; i++) {
		char ch = *p++;
		*u <<= 4;
		if (ch >= '0' && ch <= '9') * u |= ch - '0';
		else if (ch >= 'a' && ch <= 'f') * u |= ch - 'a' + 10;
		else if (ch >= 'A' && ch <= 'F') * u |= ch - 'A' + 10;
		else
			return NULL;
	}
	return p;

	/*
		//另一种写法,这种写法需要判断是否跳过空白
		char* end;
		*u = (unsigned)strtol(p, &end, 16);
		return end==p+4 ? end: NULL;
	*/
}

static void lept_encode_utf8(lept_context* c, unsigned u) {//将码点编为utf8的编码
	//TODO
	if (u <= 0x7F) {
		PUTC(c, u & 0xFF);//为什么要做 x & 0xFF 这种操作呢？u 是 unsigned 类型，一些编译器可能会警告这个转型可能会截断数据
	}
	else if (u <= 0x7FF) {
		PUTC(c, 0xC0 | ((u<<6) & 0xFF));
		PUTC(c, 0x80 | (u & 0x3F));
	}
	else if (u <= 0xFFFF) {
		PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
		PUTC(c, 0x80 | ((u >> 6) & 0x3F));
		PUTC(c, 0x80 | (u & 0x3F));
	}
	else {
		assert(u <= 0x10FFFF);
		PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
		PUTC(c, 0x80 | ((u >> 12) & 0x3F));
		PUTC(c, 0x80 | ((u >> 6) & 0x3F));
		PUTC(c, 0x80 | (u & 0x3F));
	}
}

static int lept_parse_string(lept_context* c, lept_value* v) {
	size_t head = c->top, len;
	unsigned u, u2;
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
			case 'u':
				if (!(p = lept_parse_hex4(p, &u))) STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
				if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
					if (*p++ != '\\')
						STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
					if (*p++ != 'u')
						STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
					if (!(p = lept_parse_hex4(p, &u2)))
						STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
					if (u2 < 0xDC00 || u2 > 0xDFFF)
						STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
					u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
				}
				lept_encode_utf8(c,u);
				break;
			default:
				STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);//表示无效的转义字符
			}
		case '\0':
			STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);//表示缺少标记位（即双引号）
		default:
			if ((unsigned char)ch < 0x20) {//在0-31的范围内是不合法的
				STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
			}
			PUTC(c, ch);
		}
	}
}

static int lept_parse_value(lept_context* c, lept_value* v);//声明函数

static int lept_parse_array(lept_context* c, lept_value* v) {
	size_t size = 0, head = c->top;
	int ret;
	EXPECT(c, '[');
	if (*c->json == ']') {
		c->json++;
		v->type = LEPT_ARRAY;
		v->u.a.e = NULL;
		v->u.a.size = 0;
		return LEPT_PARSE_OK;
	}
	const char* p = c->json;
	for (;;) {
		lept_value e;
		lept_init(&e);
		if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK) 
			return ret;
		memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));
		size++;
		if (*c->json == ',')
			c->json++;
		else if (*c->json == ']') {
			c->json++;
			v->type = LEPT_ARRAY;
			v->u.a.size = size;
			size *= sizeof(lept_value);
			memcpy(v->u.a.e = (lept_value*)malloc(size), lept_context_pop(c, sizeof(lept_value)), size);
			return LEPT_PARSE_OK;
		}
		else
			c->top = head;
			return LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
	}
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json){
		case '"': return lept_parse_string(c,v);
		case 'n': return lept_parse_litral(c, v, "null", LEPT_NULL);
		case 't': return lept_parse_litral(c, v, "true", LEPT_TRUE);
		case 'f': return lept_parse_litral(c, v, "false", LEPT_FALSE);
		case '\0': return LEPT_PARSE_EXPECT_VALUE;
		case '[': return lept_parse_array(c, v);
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

void lept_free(lept_value* v) {//这个函数的作用：释放v所占的内存，包括字符串，对象和数组
	assert(v != NULL);
	if (v->type == LEPT_STRING) free(v->u.s.s);
	v->type = LEPT_NULL;//释放内存后，把v的类型置空，目的是为了避免重复释放
}

int lept_get_boolean(const lept_value* v) {
	assert(v!=NULL && (v->type==LEPT_TRUE || v->type == LEPT_FALSE));
	return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
	assert(v != NULL);
	//lept_free(v);
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
	//这里的断言条件是：非空指针或0长度的字符串都是合法的
	assert(v != NULL && (s != NULL || len == 0)); //这里的代码感觉很难看得懂，重要的原因在于memcpy函数的要求
	lept_free(v);//这里释放内存的原因是：之前的字符串占的大小和这一次的大小可能不同，要重新分配内存
	v->u.s.s = (char*)malloc(len + 1);
	memcpy(v->u.s.s, s, len);//memcpy的入参有哪些要求，以及在临界条件下会出现什么效果
	v->u.s.s[len] = '\0';
	v->type = LEPT_STRING;
	v->u.s.len = len;
}

size_t lept_get_array_size(const lept_value* v) {
	assert(v!=NULL && v->type==LEPT_ARRAY);
	return v->u.a.size;
}

const lept_value* lept_get_array_element(const lept_value* v, size_t index) {
	assert(v != NULL && v->type==LEPT_ARRAY);
	assert(index < v->u.a.size);
	return &(v->u.a.e[index]);
}
