#define _CRTDBG_MAP_ALLOC
#define _CRT_SECURE_NO_DEPRECATE
#include <stdlib.h>  /*NULL, strtod(), free()*/
#include <crtdbg.h>
#include "leptjson.h"
#include <assert.h>  /*assert()*/
#include <math.h>  /*HUGE_VAL*/
#include <errno.h>  /*errno, ERANGE*/
#include <string.h> /*memcpy()*/
#include <stdio.h> /* sprintf() */

#define EXPECT(c, ch) do{ assert(*c->json == ch); c->json++;}while(0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif // LEPT_PARSE_STACK_INIT_SIZE
#define PUTC(c, ch) do{ *(char*)lept_context_push(c, sizeof(char)) = (ch);}while(0)

#ifndef LEPT_PARSE_STRINGIFY_INIT_SIZE
#define LEPT_PARSE_STRINGIFY_INIT_SIZE 256
#endif // !LEPT_PARSE_STRINGIFY_INIT_SIZE
#define PUTS(c, s, len) memcpy((char*)lept_context_push(c, len), s, len)



//Ϊʲôʹ�ö�ջ��Ϊ������������ֱ��ʹ��char*��Ϊʲô�����json����Ҫʹ��realloc��
typedef struct {
	const char* json;// ����ʹ��char* ���൱��˵json��һ��ָ�������ָ��һ���ַ�������˵����һ���ַ���
	char* stack;
	size_t size, top;//Ϊʲôstack������չ������top����ʹ��ָ�룿
}lept_context;


static void* lept_context_push(lept_context* c, size_t size) {//��������������ǿ��ٿռ�
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

static const char* lept_parse_hex4(const char* p, unsigned* u) {//���ַ�����Ϊ���
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
		//��һ��д��,����д����Ҫ�ж��Ƿ������հ�
		char* end;
		*u = (unsigned)strtol(p, &end, 16);
		return end==p+4 ? end: NULL;
	*/
}

static void lept_encode_utf8(lept_context* c, unsigned u) {//������Ϊutf8�ı���
	//TODO
	if (u <= 0x7F) {
		PUTC(c, u & 0xFF);//ΪʲôҪ�� x & 0xFF ���ֲ����أ�u �� unsigned ���ͣ�һЩ���������ܻᾯ�����ת�Ϳ��ܻ�ض�����
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

static int lept_parse_string_raw(lept_context* c, char** str, size_t* len) {
	size_t head = c->top;
	unsigned u, u2;
	EXPECT(c, '\"');
	const char* p = c->json;
	while (1) {
		char ch = *p++;
		switch (ch) {
		case '\"':
			*len = c->top - head;
			*str = (const char*)lept_context_pop(c, *len);
			c->json = p;
			return LEPT_PARSE_OK;
		case '\\':
			switch (*p++) {
			case '\"': PUTC(c, '\"'); break;
			case '\\': PUTC(c, '\\'); break;
			case 'b': PUTC(c, '\b'); break;
			case 'f': PUTC(c, '\f'); break;
			case 'n': PUTC(c, '\n'); break;
			case 'r': PUTC(c, '\r'); break;
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
				lept_encode_utf8(c, u);
				break;
			default:
				STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);//��ʾ��Ч��ת���ַ�
			}
			break;
		case '\0':
			STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);//��ʾȱ�ٱ��λ����˫���ţ�
		default:
			if ((unsigned char)ch < 0x20) {//��0-31�ķ�Χ���ǲ��Ϸ���
				STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
			}
			PUTC(c, ch);
		}
	}
}

static int lept_parse_string(lept_context* c, lept_value* v) {
	int ret;
	char* s;
	size_t len;
	if ((ret = lept_parse_string_raw(c, &s, &len)) == LEPT_PARSE_OK) {
		lept_set_string(v, s, len);
	}
	return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v);//��������

static int lept_parse_array(lept_context* c, lept_value* v) {
	size_t size = 0, head = c->top;
	int ret;
	EXPECT(c, '[');
	lept_parse_whitespace(c);
	if (*c->json == ']') {
		c->json++;
		v->type = LEPT_ARRAY;
		v->u.a.e = NULL;
		v->u.a.size = 0;
		return LEPT_PARSE_OK;
	}
	for (;;) {
		lept_value e;
		lept_init(&e);
		if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK) 
			break;
		memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));
		size++;
		lept_parse_whitespace(c);// ��������ǰ�Ŀհ�
		if (*c->json == ',') {
			c->json++;
			lept_parse_whitespace(c);// �������ź�Ŀհ�
		}
		else if (*c->json == ']') {
			c->json++;
			v->type = LEPT_ARRAY;
			v->u.a.size = size;
			size *= sizeof(lept_value);
			memcpy(v->u.a.e = (lept_value*)malloc(size), lept_context_pop(c, size), size);
			return LEPT_PARSE_OK;
		}
		else{
			ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
			break;
		}
	}
	int i;
	for (i = 0; i < size; i++) {
		lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value)));
	}
	return ret;
}

static int lept_parse_object(lept_context* c, lept_value* v) {
	int ret;
	lept_member m;
	size_t i,size=0;
	EXPECT(c, '{');
	lept_parse_whitespace(c);
	if (*c->json == '}') {
		c->json++;
		v->type = LEPT_OBJECT;
		v->u.o.m = NULL;
		v->u.o.size = 0;
		return LEPT_PARSE_OK;
	}
	for (;;) {
		/* parse key*/
		char* str;
		lept_init(&m.v);
		m.k = NULL;// ע�⣺�ṹ������ǲ����ÿյģ�����&m.v = NULL, ����Ч�ġ�
		//���ڽṹ����˵NULL���ǿ�ָ�룬��PB����û��CREATEһ���ṹ��ķ������ṹ����ڴ������������ʱ���еģ�ͬ��PBҲû��Ϊ�ṹ����ΪNULL�ķ���
		if (*c->json != '"') {
			ret = LEPT_PARSE_MISS_KEY;
			break;
		}
		if ((ret = lept_parse_string_raw(c, &str, &m.klen)) != LEPT_PARSE_OK)
			break;
		memcpy(m.k = (char*)malloc(m.klen+1), str, m.klen); //�����ַ����������Ƹ�m����
		m.k[m.klen] = '\0';

		/* parse ws colon ws*/
		lept_parse_whitespace(c);
		if (*c->json!=':') {
			ret = LEPT_PARSE_MISS_COLON;
			break;
		}
		c->json++;
		lept_parse_whitespace(c);

		/* parse value */
		if ((ret = lept_parse_value(c, &m.v)) != LEPT_PARSE_OK)
			break;
		memcpy(lept_context_push(c, sizeof(lept_member)), &m, sizeof(lept_member)); //�ѽ����õ�m�������ŵ���ջ��
		size++;

		/* parse ws [comma | right-curly-brace] ws */
		lept_parse_whitespace(c);
		if (*c->json == ',') {
			c->json++;
			lept_parse_whitespace(c);
		}
		else if (*c->json == '}') {
			c->json++;
			size_t s = size * sizeof(lept_member);
			v->u.o.size = size;
			v->type = LEPT_OBJECT;
			memcpy(v->u.o.m = (lept_member*)malloc(s), lept_context_pop(c, s), s);
			return LEPT_PARSE_OK;
		}
		else {
			ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
			break;
		}
	}

	//free(m.k);	//��֣��������ע�͵�����Ȼ�ͱ���
	for (i = 0; i < size;i++) {
		lept_member* mem = (lept_member*)lept_context_pop(c, sizeof(lept_member));
		free(mem->k);
		lept_free(&mem->v);
	}
	v->type = LEPT_NULL;
	return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json){
		case '"': return lept_parse_string(c,v);
		case 'n': return lept_parse_litral(c, v, "null", LEPT_NULL);
		case 't': return lept_parse_litral(c, v, "true", LEPT_TRUE);
		case 'f': return lept_parse_litral(c, v, "false", LEPT_FALSE);
		case '\0': return LEPT_PARSE_EXPECT_VALUE;
		case '[': return lept_parse_array(c, v);
		case '{': return lept_parse_object(c, v);
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
	size_t i;
	switch (v->type) {
		case LEPT_STRING:
			free(v->u.s.s);
			break;
		case LEPT_ARRAY:
			for (i = 0; i < v->u.a.size; i++) {
				lept_free(&v->u.a.e[i]);
			}
			free(v->u.a.e);
			break;
		case LEPT_OBJECT:
			for (i = 0; i < v->u.o.size;i++) {
				free(v->u.o.m[i].k);
				lept_free(&v->u.o.m[i].v);
			}
			free(v->u.o.m);
		default:
			break;
	}
	v->type = LEPT_NULL;//�ͷ��ڴ�󣬰�v�������ÿգ�Ŀ����Ϊ�˱����ظ��ͷ�
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
	//����Ķ��������ǣ��ǿ�ָ���0���ȵ��ַ������ǺϷ���
	assert(v != NULL && (s != NULL || len == 0)); //����Ĵ���о����ѿ��ö�����Ҫ��ԭ������memcpy������Ҫ��
	lept_free(v);//�����ͷ��ڴ��ԭ���ǣ�֮ǰ���ַ���ռ�Ĵ�С����һ�εĴ�С���ܲ�ͬ��Ҫ���·����ڴ�
	v->u.s.s = (char*)malloc(len + 1);
	memcpy(v->u.s.s, s, len);//memcpy���������ЩҪ���Լ����ٽ������»����ʲôЧ��
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

size_t lept_get_object_size(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	return v->u.o.size;
}

const char* lept_get_object_key(const lept_value * v, size_t index) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	assert(index<v->u.o.size);
	return v->u.o.m[index].k;
}

size_t lept_get_object_key_length(const lept_value * v, size_t index) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	assert(index < v->u.o.size);
	return v->u.o.m[index].klen;
}

const lept_value* lept_get_object_value(const lept_value * v, size_t index) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	assert(index < v->u.o.size);
	return &v->u.o.m[index].v;
}

static void lept_stringify_string(lept_context* c, const char* s, size_t len){
	//Ϊʲô hex_digits �����ַ��������� "0123456789ABCDEF"����ʵ�ǿ��Եģ��������˷� 1 ���ֽڣ�ʵ�������ݶ�����ܻ��˷� 4 ������ࣩ
	static const char hex_digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	assert(s != NULL);
	size_t i,size;
	char* head, *p;
	p = head = lept_context_push(c, size=len*6+2); //Ԥ�ȷ����㹻���ڴ棬ÿ�μ����ַ��Ͳ������������ˡ�
	//�������ڴ���㹻�أ����ǿ��Կ�����ÿ���ַ������������ʽ�� \u00XX��ռ 6 ���ַ����ټ���ǰ������˫���ţ�Ҳ���ǹ� len * 6 + 2 ������ַ�
	*p++ = '"';
	for (i=0; i<len; i++) {
		unsigned char ch = (unsigned char)s[i];
		switch (ch) {
			//����ַ����е��ַ���ת���ַ�����ô���ɵ�ʱ��Ҫ���� '\'
		case '\"': *p++ = '\\'; *p++ = '\"'; break;
		case '\\': *p++ = '\\'; *p++ = '\\'; break;
		case '\b': *p++ = '\\'; *p++ = 'b';  break;
		case '\f': *p++ = '\\'; *p++ = 'f';  break;
		case '\n': *p++ = '\\'; *p++ = 'n';  break;
		case '\r': *p++ = '\\'; *p++ = 'r';  break;
		case '\t': *p++ = '\\'; *p++ = 't';  break;
			default:
				if (ch < 0x20) { // �������ַ�С��32�Ļ�����Ҫ��\u00xx�ķ�ʽ�����
					//������ printf() �ڽ�����ʽ�Ŀ�����
					*p++ = '\\'; *p++ = 'u'; *p++ = '0'; *p++ = '0';
					*p++ = hex_digits[ch >> 4]; //��ȡǰ��4λ
					*p++ = hex_digits[ch & 15]; //��ȡ����4λ
				}
				else
					*p++ = s[i];
			}
	}
	*p++ = '"';
	c->top -= size - (p - head);
}

static void lept_stringify_value(lept_context* c, const lept_value* v) {
	size_t i;
	switch (v->type) {
		case LEPT_NULL: PUTS(c, "null", 4); break;
		case LEPT_FALSE: PUTS(c, "false", 5); break;
		case LEPT_TRUE: PUTS(c, "true", 4); break;
		case LEPT_NUMBER: c->top -= 32 - sprintf_s(lept_context_push(c, 32), 32, "%.17g", v->u.n); break; //�Ѹ�ʽ�����ַ����ŵ���������
		case LEPT_STRING: lept_stringify_string(c, v->u.s.s, v->u.s.len); break;
		case LEPT_ARRAY:
			PUTC(c, '[');
			for (i = 0; i < v->u.a.size; i++) {
				if (i > 0)
					PUTC(c, ',');
				lept_stringify_value(c, &v->u.a.e[i]);
			}
			PUTC(c, ']');
			break;
		case LEPT_OBJECT:
			PUTC(c, '{');
			for (i = 0; i < v->u.o.size; i++) {
				if (i > 0)
					PUTC(c, ',');
				lept_stringify_string(c, v->u.o.m[i].k, v->u.o.m[i].klen);
				PUTC(c, ':');
				lept_stringify_value(c, &v->u.o.m[i].v);
			}
			PUTC(c, '}');
			break;
		default:
			assert(0 && "invalid type");
	}
}

char* lept_stringify(const lept_value* v, size_t* length) {
	lept_context c;
	assert(v != NULL);
	c.stack = (char*)malloc(c.size = LEPT_PARSE_STRINGIFY_INIT_SIZE);
	c.top = 0;
	lept_stringify_value(&c, v);
	if (length)
		* length = c.top;
	PUTC(&c, '\0');
	return c.stack;
}

size_t lept_find_object_index(const lept_value* v, const char* key, size_t klen) {
	size_t i;
	assert(v!=NULL && v->type==LEPT_OBJECT && key!=NULL);
	for (i = 0; i < v->u.o.size; i++)
		if (v->u.o.m[i].klen == klen && memcmp(v->u.o.m[i].k, key, klen) == 0)
			return i;
	return LEPT_KEY_NOT_EXIST;
}

const lept_value* lept_find_object_value(const lept_value* v, const char* key, size_t klen) {
	size_t index = lept_find_object_index(v, key, klen);
	return index != LEPT_KEY_NOT_EXIST ? &v->u.o.m[index].v : NULL;
}