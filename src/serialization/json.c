#include <assert.h>
#include "./json.h"

#define SER_VALIDATE(call) do { if (!(call)) { return false; } } while (0)

typedef struct Test {
	const int  * * * * ids;
	int  i;
	float  f;
	const long double  dl;
} Test;
bool serializer_Test_to_json(Serializer *p_ser, const void *p_val) {
	assert(NULL != p_val);

	const Test *tmp = (const Test*)p_val;

	SER_VALIDATE(serializer_json_start_field(p_ser, "ids"));
	SER_VALIDATE(serializer_int_to_json(p_ser, ****tmp->ids));
	SER_VALIDATE(serializer_json_end_field(p_ser));

	SER_VALIDATE(serializer_json_start_field(p_ser, "i"));
	SER_VALIDATE(serializer_int_to_json(p_ser, tmp->i));
	SER_VALIDATE(serializer_json_end_field(p_ser));

	SER_VALIDATE(serializer_json_start_field(p_ser, "f"));
	SER_VALIDATE(serializer_float_to_json(p_ser, tmp->f));
	SER_VALIDATE(serializer_json_end_field(p_ser));

	SER_VALIDATE(serializer_json_start_field(p_ser, "dl"));
	SER_VALIDATE(serializer_long_double_to_json(p_ser, tmp->dl));
	SER_VALIDATE(serializer_json_end_field(p_ser));

	return true;
}

typedef struct Test2 {
	Test  * arr;
	unsigned int  arr_count;
	void  * v;
} Test2;
bool cb_void_to_json(Serializer *p_ser, const void *value);
bool cb_json_to_void(Serializer *p_ser, void *value);
bool serializer_Test2_to_json(Serializer *p_ser, const void *p_val) {
	assert(NULL != p_val);

	const Test2 *tmp = (const Test2*)p_val;

	SER_VALIDATE(serializer_json_start_field(p_ser, "arr"));
	SER_VALIDATE(serializer_json_start_array(p_ser));
	for (size_t i = 0; i < tmp->arr_count; ++i) {
		SER_VALIDATE(serializer_Test_to_json(p_ser, tmp->arr + i));
	}
	SER_VALIDATE(serializer_json_end_array(p_ser));
	SER_VALIDATE(serializer_json_end_field(p_ser));

	SER_VALIDATE(serializer_json_start_field(p_ser, "v"));
	SER_VALIDATE(cb_void_to_json(p_ser, tmp->v));
	SER_VALIDATE(serializer_json_end_field(p_ser));

	return true;
}

