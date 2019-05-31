#ifndef LEPTJSON_H__
#define LEPTJSON_H__
#include <stddef.h>  /*size_t*/

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct {
	union {
		struct { char* s; size_t len; } s;
		double n;
	} u;
	lept_type type;
}lept_value;

enum {
	LEPT_PARSE_OK = 0,
	LEPT_PARSE_EXPECT_VALUE,
	LEPT_PARSE_INVALID_VALUE,
	LEPT_PARSE_ROOT_NOT_SINGULAR,
	LEPT_PARSE_NUMBER_TOO_BIG
};

#define lept_init(v) do{ (v)->type = LEPT_NULL; } while(0) //初始化的作用：在所有的set和get函数中，第一个就是对v进行判空，所以v一定要初始化

int lept_parse(lept_value* v, const char* json);

void lept_free(lept_value* v); //释放动态生成的字符串所占的存储

lept_type lept_get_type(const lept_value* v);

#define lept_set_null(v) lept_free(v)

int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, int b);

double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

#endif // !LEPTJSON_H__

