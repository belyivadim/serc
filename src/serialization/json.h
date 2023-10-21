#ifndef __SERC_JSON_H__
#define __SERC_JSON_H__
#include <stdbool.h>
#include "./primitives.h"

bool serializer_Test_to_json(Serializer *p_ser, const void *p_val);
bool serializer_Test2_to_json(Serializer *p_ser, const void *p_val);
#endif // !__SERC_JSON_H__
