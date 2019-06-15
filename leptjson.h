#ifndef LEPTJSON_H__
#define LEPTJSON_H__
#include <stddef.h>  /*size_t*/

#define LEPT_KEY_NOT_EXIST ((size_t)-1)

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct lept_value lept_value;//lept_value��ʹ�����������͵�ָ�룬���Ǳ���ǰ��������forward declare��������
typedef struct lept_member lept_member;
struct lept_value{
	union {
		struct { lept_member* m; size_t size;}o; //ʹ�ö�̬�����ʾ����
		struct { lept_value* e; size_t size; }a; //ʹ�����飨������������ʾjson�����飬�����size���������Ԫ�ظ���
		struct { char* s; size_t len; } s;
		double n;
	} u;
	lept_type type;
};

struct lept_member {
	char* k;
	size_t klen; //ʹ�ø��ֶα�ʾ���ĳ��ȣ�ԭ���Ǽ����ܰ������ַ�(\u0000)
	lept_value v; //Ϊʲô����ʹ��lept_value������lept_value* ?  ��Ϊһ��memberֻ��Ӧһ��lept_value
};

enum {
	LEPT_PARSE_OK = 0,
	LEPT_PARSE_EXPECT_VALUE,
	LEPT_PARSE_INVALID_VALUE,
	LEPT_PARSE_ROOT_NOT_SINGULAR,
	LEPT_PARSE_NUMBER_TOO_BIG,
	LEPT_PARSE_MISS_QUOTATION_MARK,	//ȱ�ٺ����˫����
	LEPT_PARSE_INVALID_STRING_ESCAPE,	//��Ч��ת���ַ�
	LEPT_PARSE_INVALID_STRING_CHAR,	//���Ϸ����ַ�����㷶Χ��0-31֮��Ĳ��Ϸ���
	LEPT_PARSE_INVALID_UNICODE_HEX,	//��Ч��unicode 16�����ַ���\uxxxx������x����0-f֮�䣩
	LEPT_PARSE_INVALID_UNICODE_SURROGATE,	//��Ч��unicode����ԣ������ϴ���Ե�Ҫ��
	LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,	//ȱ�ٶ��Ż�����
	LEPT_PARSE_MISS_KEY, //object��keyû�н�������
	LEPT_PARSE_MISS_COLON, //object�е�key����û��ð��
	LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, //object�н����ٶ��Ż�����������
};

#define lept_init(v) do{ (v)->type = LEPT_NULL; } while(0) //��ʼ�������ã������е�set��get�����У���һ�����Ƕ�v�����пգ�����vһ��Ҫ��ʼ��

int lept_parse(lept_value* v, const char* json);
char* lept_stringify(const lept_value* v, size_t* length); //����lept_value����json�ַ�����length�����ǿ�ѡ�ģ�����NULL��ʾ���ԡ�ʹ�÷���Ҫ��free()�ͷ��ڴ档

void lept_copy(lept_value* dst, const lept_value* src); // ��ȸ���
void lept_move(lept_value* dst, lept_value* src); //�ƶ�
void lept_swap(lept_value* lhs, lept_value* rhs); //����

void lept_free(lept_value* v); //�ͷŶ�̬���ɵ��ַ�����ռ�Ĵ洢

lept_type lept_get_type(const lept_value* v);
int lept_is_equal(const lept_value* lhs, const lept_value* rhs); //�ж�����lept_value�Ƿ���ͬ

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

size_t lept_get_object_size(const lept_value* v); //��ȡobject��Ԫ�صĸ���
const char* lept_get_object_key(const lept_value* v, size_t index); // ����index���ҵ���Ӧ��key
size_t lept_get_object_key_length(const lept_value* v, size_t index); //����index���Ҷ�Ӧkey�ĳ���
const lept_value* lept_get_object_value(const lept_value* v, size_t index); // ����index���Ҷ�Ӧ��value
size_t lept_find_object_index(const lept_value* v, const char* key, size_t len); // ����key���ҵ�object�е�index
const lept_value* lept_find_object_value(const lept_value* v, const char* key, size_t klen); //����key���ҵ�objec�е�value
lept_value* lept_set_object_value(lept_value* v, const char* key, size_t klen); //���������һ��key�������ظ�key��Ӧvalue
void lept_remove_object_value(lept_value* v, size_t index);

#endif // !LEPTJSON_H__