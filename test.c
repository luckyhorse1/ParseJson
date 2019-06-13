#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>|
#include <stdio.h>
#include "leptjson.h"
#include <string.h>

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format)\
	do{\
		test_count++;\
		if(equality)\
			test_pass++;\
		else{\
			fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
		}\
	} while (0)

#define EXPECT_EQ_INT(expect, actual)  EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")

//这里为什么使用sizeof而不是strlen？sizeof获取的是该字符串占用的内存大小，包括'\0'。而strlen获取的是第一个字符到'\0'之前的字符个数，不包括'\0'。
#define EXPECT_EQ_STRING(expect, actual, alength)\
	EXPECT_EQ_BASE((sizeof(expect)-1 == alength) && memcmp(expect, actual, alength)==0, expect, actual, "%s")

#define EXPECT_EQ_TRUE(actual)  EXPECT_EQ_BASE((actual)!=0, "true", "false", "%s")
#define EXPECT_EQ_FALSE(actual)  EXPECT_EQ_BASE((actual)==0, "false", "true", "%s")

#if defined(_MSC_VER) //使用条件编译区分编译器
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect)==(actual), expect, actual, "%Iu");
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect)==(actual), expect, actual, "%zu");
#endif

#define TEST_NUMBER(expect, json)\
	do{\
		lept_value v;\
		v.type = LEPT_FALSE;\
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
		EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));\
		EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));\
	} while(0)

#define TEST_STRING(expect, json)\
	do{\
		lept_value v;\
		v.type = LEPT_FALSE;\
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
		EXPECT_EQ_INT(LEPT_STRING, lept_get_type(&v));\
		EXPECT_EQ_STRING(expect, lept_get_string(&v), lept_get_string_length(&v));\
	}while(0)

#define TEST_ERROR(error, json)\
	do{\
		lept_value v;\
		v.type = LEPT_FALSE;\
		EXPECT_EQ_INT(error, lept_parse(&v, json));\
		EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));\
	}while(0)

static void test_parse_null() {
	lept_value v;
	v.type = LEPT_FALSE;  //这里仅是赋一个初始值而已，没有其他的意义。经过lept_parse函数，该变量都会变为初始值为LEPT_NULL。
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse_true() {
	lept_value v;
	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true"));
	EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
}

static void test_parse_false() {
	lept_value v;
	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false"));
}

static void test_parse_number() {
	TEST_NUMBER(0.0, "0");
	TEST_NUMBER(0.0, "-0");
	TEST_NUMBER(0.0, "-0.0");
	TEST_NUMBER(1.0, "1");
	TEST_NUMBER(-1.0, "-1");
	TEST_NUMBER(1.5, "1.5");
	TEST_NUMBER(-1.5, "-1.5");
	TEST_NUMBER(3.1416, "3.1416");
	TEST_NUMBER(1E10, "1E10");
	TEST_NUMBER(1e10, "1e10");
	TEST_NUMBER(1E+10, "1E+10");
	TEST_NUMBER(1E-10, "1E-10");
	TEST_NUMBER(-1E10, "-1E10");
	TEST_NUMBER(-1e10, "-1e10");
	TEST_NUMBER(-1E+10, "-1E+10");
	TEST_NUMBER(-1E-10, "-1E-10");
	TEST_NUMBER(1.234E+10, "1.234E+10");
	TEST_NUMBER(1.234E-10, "1.234E-10");
	TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

	TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
	TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
	TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
	TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
	TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
	TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
	TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
	TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
	TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

static void test_parse_expect_value() {//检测字符串是否只有空白
	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?");

	#if 0
		TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");
		TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
		TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123");
		TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");
		TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
		TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
		TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
		TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
	#endif
}

static void test_parse_root_not_singular() {
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null a");

	/* invalid number */
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
	TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
	TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_string() {
	TEST_STRING("abc", "\"abc\"");
	TEST_STRING("", "\"\"");
}

static void test_parse_missing_quotation_mark() {
	TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"");
	TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_parse_invalid_unicode_hex() {
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
}

static void test_parse_invalide_unicode_surrogate() {
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_array() {
	lept_value v;
	lept_init(&v);
	//EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[]"));
	//EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
	//EXPECT_EQ_SIZE_T(0, lept_get_array_size(&v));

	//lept_init(&v);
#if 0
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
	EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
	EXPECT_EQ_SIZE_T(5, lept_get_array_size(&v));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(lept_get_array_element(&v,0)));
	EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(lept_get_array_element(&v, 1)));
	EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(lept_get_array_element(&v, 2)));
	EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_array_element(&v, 3)));
	EXPECT_EQ_INT(LEPT_STRING, lept_get_type(lept_get_array_element(&v, 4)));
	EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_array_element(&v, 3)));
	EXPECT_EQ_STRING("abc", lept_get_string(lept_get_array_element(&v, 4)), lept_get_string_length(lept_get_array_element(&v, 4)));
#endif

#if 1
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ [] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
	EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
	EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v));
	int i,j;
	for (i = 0; i < 4; i++) {
		lept_value* a = lept_get_array_element(&v, i);
		EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(a));
		EXPECT_EQ_INT(i, lept_get_array_size(a));
		for (j = 0; j < i; j++) {
			EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_array_element(a, j)));
			EXPECT_EQ_INT(j, lept_get_number(lept_get_array_element(a, j)));
		}
	}
#endif
	lept_free(&v);
}

static void test_parse_miss_comma_or_square_bracket() {
#if 1
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
#endif
}

static void test_parse_object() {
	lept_value v;
	size_t i;

	lept_init(&v);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, " { } "));
	EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
	EXPECT_EQ_SIZE_T(0, lept_get_object_size(&v));
	lept_free(&v);

	lept_init(&v);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,
		" { "
		"\"n\" : null , "
		"\"f\" : false , "
		"\"t\" : true , "
		"\"i\" : 123 , "
		"\"s\" : \"abc\", "
		"\"a\" : [ 1, 2, 3 ],"
		"\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
		" } "
	));
	EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
	EXPECT_EQ_SIZE_T(7, lept_get_object_size(&v));
	EXPECT_EQ_STRING("n", lept_get_object_key(&v, 0), lept_get_object_key_length(&v, 0));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(lept_get_object_value(&v, 0)));
	EXPECT_EQ_STRING("f", lept_get_object_key(&v, 1), lept_get_object_key_length(&v, 1));
	EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(lept_get_object_value(&v, 1)));
	EXPECT_EQ_STRING("t", lept_get_object_key(&v, 2), lept_get_object_key_length(&v, 2));
	EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(lept_get_object_value(&v, 2)));
	EXPECT_EQ_STRING("i", lept_get_object_key(&v, 3), lept_get_object_key_length(&v, 3));
	EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_object_value(&v, 3)));
	EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_object_value(&v, 3)));
	EXPECT_EQ_STRING("s", lept_get_object_key(&v, 4), lept_get_object_key_length(&v, 4));
	EXPECT_EQ_INT(LEPT_STRING, lept_get_type(lept_get_object_value(&v, 4)));
	EXPECT_EQ_STRING("abc", lept_get_string(lept_get_object_value(&v, 4)), lept_get_string_length(lept_get_object_value(&v, 4)));
	EXPECT_EQ_STRING("a", lept_get_object_key(&v, 5), lept_get_object_key_length(&v, 5));
	EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(lept_get_object_value(&v, 5)));
	EXPECT_EQ_SIZE_T(3, lept_get_array_size(lept_get_object_value(&v, 5)));
	for (i = 0; i < 3; i++) {
		lept_value* e = lept_get_array_element(lept_get_object_value(&v, 5), i);
		EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
		EXPECT_EQ_DOUBLE(i + 1.0, lept_get_number(e));
	}
	EXPECT_EQ_STRING("o", lept_get_object_key(&v, 6), lept_get_object_key_length(&v, 6));
	{
		lept_value* o = lept_get_object_value(&v, 6);
		EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(o));
		for (i = 0; i < 3; i++) {
			lept_value* ov = lept_get_object_value(o, i);
			EXPECT_EQ_TRUE('1' + i == lept_get_object_key(o, i)[0]);
			EXPECT_EQ_SIZE_T(1, lept_get_object_key_length(o, i));
			EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(ov));
			EXPECT_EQ_DOUBLE(i + 1.0, lept_get_number(ov));
		}
	}
	lept_free(&v);
}

static void test_parse_miss_colon() {
	TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\"}");
	TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}


static void test_parse() {
	//test_parse_null();
	//test_parse_true();
	//test_parse_false();
	//test_parse_number();
	//test_parse_string();
	//test_parse_array();
	test_parse_object();

	//test_parse_expect_value();
	//test_parse_invalid_value();
	//test_parse_root_not_singular();
	//test_parse_number_too_big();
	//test_parse_missing_quotation_mark();
	//test_parse_invalid_string_escape();
	//test_parse_invalid_string_char();
	//test_parse_invalid_unicode_hex();
	//test_parse_invalide_unicode_surrogate();
	//test_parse_miss_comma_or_square_bracket();
	//test_parse_miss_colon();
	//test_parse_miss_comma_or_curly_bracket();
}

#define TEST_ROUNDTRIP(json) \
	do{\
		lept_value v;\
		size_t length;\
		char* json2;\
		lept_init(&v);\
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
		json2 = lept_stringify(&v, &length);\
		EXPECT_EQ_STRING(json, json2, length);\
		lept_free(&v);\
		free(json2);\
	}while(0)

static void test_stringify_number() {
	TEST_ROUNDTRIP("0");
	TEST_ROUNDTRIP("-0");
	TEST_ROUNDTRIP("1");
	TEST_ROUNDTRIP("-1");
	TEST_ROUNDTRIP("1.5");
	TEST_ROUNDTRIP("-1.5");
	TEST_ROUNDTRIP("3.25");
	TEST_ROUNDTRIP("1e+20");
	TEST_ROUNDTRIP("1.234e+20");
	TEST_ROUNDTRIP("1.234e-20");

	TEST_ROUNDTRIP("1.0000000000000002"); /* the smallest number > 1 */
	TEST_ROUNDTRIP("4.9406564584124654e-324"); /* minimum denormal */
	TEST_ROUNDTRIP("-4.9406564584124654e-324");
	TEST_ROUNDTRIP("2.2250738585072009e-308");  /* Max subnormal double */
	TEST_ROUNDTRIP("-2.2250738585072009e-308");
	TEST_ROUNDTRIP("2.2250738585072014e-308");  /* Min normal positive double */
	TEST_ROUNDTRIP("-2.2250738585072014e-308");
	TEST_ROUNDTRIP("1.7976931348623157e+308");  /* Max double */
	TEST_ROUNDTRIP("-1.7976931348623157e+308");
}

static void test_stringify_string() {
	//TEST_ROUNDTRIP("\"\"");
	//TEST_ROUNDTRIP("\"Hello\"");
	//TEST_ROUNDTRIP("\"Hello\\nWorld\"");
	TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
	//TEST_ROUNDTRIP("\"Hello\\u0000World\"");
}

static void test_stringify_array() {
	TEST_ROUNDTRIP("[]");
	TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
}

static void test_stringify_object() {
	TEST_ROUNDTRIP("{}");
	TEST_ROUNDTRIP("{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
}

static void test_stringify() {
	//TEST_ROUNDTRIP("null");
	//TEST_ROUNDTRIP("false");
	//TEST_ROUNDTRIP("true");
	//test_stringify_number();
	//test_stringify_string();
	//test_stringify_array();
	test_stringify_object();
}

static void test_access_null() {
	lept_value v;
	lept_init(&v);
	lept_set_null(&v);
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
	lept_free(&v);
}

static void test_access_boolean() {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "a", 1);
	lept_set_boolean(&v, 1);
	EXPECT_EQ_TRUE(lept_get_boolean(&v));
	lept_set_boolean(&v, 0);
	EXPECT_EQ_FALSE(lept_get_boolean(&v));
	//lept_free(&v);
}

static void test_access_number() {
	lept_value v;
	lept_init(&v);
	lept_set_number(&v, 2.0);
	EXPECT_EQ_DOUBLE(2.0, lept_get_number(&v));
	lept_free(&v);
}

static void test_access_string() {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "", 0);
	EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v));
	lept_set_string(&v, "Hello", 5);
	EXPECT_EQ_STRING("Hello", lept_get_string(&v), lept_get_string_length(&v));
	lept_free(&v);
}

static void test_access() {
	//test_access_null();
	test_access_boolean();
	//test_access_number();
}

static void test_find_index() {
	lept_value v;
	lept_init(&v);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "{\"name\" : \"xiaoma\", \"sex\" : 1 }"));
	EXPECT_EQ_SIZE_T(0, lept_find_object_index(&v, "name", 4));
	const lept_value* v2 = lept_find_object_value(&v, "name", 4);
	EXPECT_EQ_STRING("xiaoma", v2->u.s.s, v2->u.s.len);
	printf("%s\n", lept_get_string(v2));
	lept_free(&v);
}

#define TEST_EQUAL(json1, json2, equality) \
	do{\
		lept_value v1, v2;\
		lept_init(&v1); \
		lept_init(&v2); \
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v1, json1)); \
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v2, json2));\
		EXPECT_EQ_INT(equality, lept_is_equal(&v1, &v2));\
		lept_free(&v1);\
		lept_free(&v2);\
	}while(0)

static void test_equal() {
	TEST_EQUAL("true", "true", 1);
	TEST_EQUAL("true", "false", 0);
	TEST_EQUAL("false", "false", 1);
	TEST_EQUAL("null", "null", 1);
	TEST_EQUAL("null", "0", 0);
	TEST_EQUAL("123", "123", 1);
	TEST_EQUAL("123", "456", 0);
	TEST_EQUAL("\"abc\"", "\"abc\"", 1);
	TEST_EQUAL("\"abc\"", "\"abcd\"", 0);
	TEST_EQUAL("[]", "[]", 1);
	TEST_EQUAL("[]", "null", 0);
	TEST_EQUAL("[1,2,3]", "[1,2,3]", 1);
	TEST_EQUAL("[1,2,3]", "[1,2,3,4]", 0);
	TEST_EQUAL("[[]]", "[[]]", 1);
	TEST_EQUAL("{}", "{}", 1);
	TEST_EQUAL("{}", "null", 0);
	TEST_EQUAL("{}", "[]", 0);
	TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2}", 1);
	TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"b\":2,\"a\":1}", 1);
	TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":3}", 0);
	TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2,\"c\":3}", 0);
	TEST_EQUAL("{\"a\":{\"b\":{\"c\":{}}}}", "{\"a\":{\"b\":{\"c\":{}}}}", 1);
	TEST_EQUAL("{\"a\":{\"b\":{\"c\":{}}}}", "{\"a\":{\"b\":{\"c\":[]}}}", 0);
}

int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//test_parse();
	//test_access();
	//test_stringify();
	//test_find_index();
	test_equal();
	printf("%d %d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
	return main_ret;
}