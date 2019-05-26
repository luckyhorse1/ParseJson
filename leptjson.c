#include "leptjson.h"
#include <assert.h>
#include <math.h>

#define EXPECT(c, ch) do{ assert(*c->json == ch); c->json++;}while(0)

typedef struct {
	const char* json;// ����ʹ��char* ���൱��˵json��һ��ָ�������ָ��һ���ַ�������˵����һ���ַ���
}lept_context;


static void lept_parse_whitespace(lept_context* c) {
	const char *p = c->json; 
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		p++;
	c->json = p;
}

static int lept_parse_null(lept_context* c, lept_value* v) {
	EXPECT(c, 'n');// �����ж϶���һ����ĸ�Բ���
	if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l' || c->json[3] != '\0')//�������ж������ÿһ���ַ��Բ���
		return LEPT_PARSE_INVALID_VALUE;
	c->json += 3;// ��仰û����
	v->type = LEPT_NULL;
	return LEPT_PARSE_OK;
}

static int lept_parse_true(lept_context* c, lept_value* v) {
	EXPECT(c, 't');
	if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e' || c->json[3] != '\0')
		return LEPT_PARSE_INVALID_VALUE;
	c->json += 3;
	v->type = LEPT_TRUE;
	return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context* c, lept_value* v) {
	EXPECT(c, 'f');
	if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e' || c->json[4] !='\0')
		return LEPT_PARSE_INVALID_VALUE;
	c->json += 4;
	v->type = LEPT_FALSE;
	return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
	int sign = 1;
	double left = 0.0;
	double right = 0.0;
	int left_num = 0;//������ֵ�λ��
	int e = 0;
	int e_num = 0;//e��λ��
	int e_sign = 1;

	// ȷ�����ֵķ���
	if (c->json[0] == '-') {
		sign = -1;
		c->json++;
	}
	
	//ȷ��left
	if (c->json[0] < '0' || c->json[0] > '9') return LEPT_PARSE_INVALID_VALUE; // �������ַ����������ֿ�ͷ��ֱ�ӷ�����Ч
	else if (c->json[0] == '0') {// ��0��ͷ���������⴦��
		if (c->json[1] == '\0') {//��0��ͷ������û���ַ���ֱ�ӷ���0
			v->type = LEPT_NUMBER;
			v->n = 0.0;
			c->json++;
			return LEPT_PARSE_OK;
		}
		if (c->json[1] != '.') return LEPT_PARSE_INVALID_VALUE;//�����������0��ͷ�������治��С���㣬����Ϊ�Ǵ����
		c->json++;//cָ������ָ��С����
	}
	else {//�Է�0���ֿ�ͷ
		int count = 0;
		const char* p = c->json;
		while (*p >= '0' && *p <= '9') {
			p++;
			count++;
		}
		int i;
		for (i = 0; i < count; i++) {
			left += (c->json[i] - '0') * pow(10, count - i - 1);
		}
		c->json += count;
		left_num = count;
	}

	if (c->json[0] == '\0') {
		v->type = LEPT_NUMBER;
		v->n = sign*left;
		return LEPT_PARSE_OK;
	}

	//ȷ��right
	if ( c->json[0] == '.') {
		c->json++;
		int count = 0;
		const char* p = c->json;
		while (*p >= '0' && *p <= '9') {
			count++;
			right += (*p - '0') * pow(10, -count);
			p++;
		}
		if (count == 0) return LEPT_PARSE_INVALID_VALUE;//ǰһ����ĸ��С���㣬�������ĸ��������
		c->json += count;
	}
	else if (c->json[0] != 'e' && c->json[0] != 'E') {
		return LEPT_PARSE_INVALID_VALUE;
	}

	if (c->json[0] == '\0') {
		v->type = LEPT_NUMBER;
		v->n = sign*(left+right);
		return LEPT_PARSE_OK;
	}
	
	//ȷ��e
	if (c->json[0] == 'e' || c->json[0] == 'E') {
		c->json++;
		if (c->json[0] == '+') {
			e_sign = 1;
			c->json++;
		}
		else if (c->json[0] == '-') {
			e_sign = -1;
			c->json++;
		}

		int count = 0;
		const char* p = c->json;
		while (*p >= '0' && *p <= '9') {
			p++;
			count++;
		}
		if (*p != '\0') return LEPT_PARSE_INVALID_VALUE;
		else if (count + left_num >= 309) return LEPT_PARSE_NUMBER_TOO_BIG;
		else {
			int i;
			for (i = 0; i < count; i++) {
				e += (c->json[i] - '0') * pow(10, count-i-1);
			}
			c->json += count;
			e_num = count;
		}
	}
	else{
		return LEPT_PARSE_INVALID_VALUE;
	}

	v->type = LEPT_NUMBER;
	v->n = sign*(left + right) * pow(10, e_sign*e);
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
	assert(v != NULL);
	c.json = json;
	v->type = LEPT_NULL;
	lept_parse_whitespace(&c);
	
	//��δ����Ŀ�ģ�����Ƿ��������ַ��������ֻ��һ���ַ�������ȥ������Ŀհ�
	const char *p = (&c)->json;
	while (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '\0')
		p++;
	if(*p != '\0') {
		char* q = p;//��ǵ�һ���հ׵�λ��
		while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
			p++;
		if (*p != '\0') return LEPT_PARSE_ROOT_NOT_SINGULAR;
		else {
			*q = '\0';//ȥ������հ�
			lept_parse_value(&c, v);
		}
	}
	return lept_parse_value(&c, v);
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}

double lept_get_number(const lept_value* v) {
	assert(v != NULL);
	return v->n;
}