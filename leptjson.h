#ifndef LEPTJSON_H__
#define LEPTJSON_H__
#include <stddef.h>  /*size_t*/

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct lept_value lept_value;//lept_value内使用了自身类型的指针，我们必须前向声明（forward declare）此类型
typedef struct lept_member lept_member;
struct lept_value{
	union {
		struct { lept_member* m; size_t size;}o; //使用动态数组表示对象
		struct { lept_value* e; size_t size; }a; //使用数组（而不是链表）表示json的数组，这里的size代表数组的元素个数
		struct { char* s; size_t len; } s;
		double n;
	} u;
	lept_type type;
};

struct lept_member {
	char* key;
	size_t klen; //使用该字段表示键的长度，原因是键可能包含空字符(\u0000)
	lept_value v;
};

enum {
	LEPT_PARSE_OK = 0,
	LEPT_PARSE_EXPECT_VALUE,
	LEPT_PARSE_INVALID_VALUE,
	LEPT_PARSE_ROOT_NOT_SINGULAR,
	LEPT_PARSE_NUMBER_TOO_BIG,
	LEPT_PARSE_MISS_QUOTATION_MARK,	//缺少后面的双引号
	LEPT_PARSE_INVALID_STRING_ESCAPE,	//无效的转义字符
	LEPT_PARSE_INVALID_STRING_CHAR,	//不合法的字符（码点范围在0-31之间的不合法）
	LEPT_PARSE_INVALID_UNICODE_HEX,	//无效的unicode 16进制字符（\uxxxx，其中x不在0-f之间）
	LEPT_PARSE_INVALID_UNICODE_SURROGATE,	//无效的unicode代理对（不符合代理对的要求）
	LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET	//缺少逗号或方括号
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

size_t lept_get_array_size(const lept_value* v);
const lept_value* lept_get_array_element(const lept_value* v, size_t index);

size_t lept_get_object_size(const lept_value* v);
const char* lept_get_object_key(const lept_value* v, size_t index);
size_t lept_get_object_key_length(const lept_value* v, size_t index);
const lept_value* lept_get_object_value(const lept_value* v, size_t index);
#endif // !LEPTJSON_H__