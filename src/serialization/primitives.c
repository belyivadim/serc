#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <float.h>

#include "primitives.h"

#define SER_GROW_FACTOR 2
#define SER_INIT_CAPACITY 64

#define SER_VALIDATE(call) do { if (!(call)) { return false; } } while (0)


// ----------------- | PRIVATE |
static bool serializer_data_maybe_expand(Serializer *p_ser) {
  assert(NULL != p_ser);

  if (p_ser->count == p_ser->capacity) {
    p_ser->capacity *= SER_GROW_FACTOR;
    char *tmp = (char*)realloc(p_ser->data, p_ser->capacity * sizeof(char));
    if (NULL == tmp) {
      return false;
    }
    p_ser->data = tmp;
  }

  return true;
}

static bool serializer_init(Serializer *p_ser) {
  assert(NULL != p_ser);
  
  p_ser->data = (char*)malloc(SER_INIT_CAPACITY * sizeof(char));
  if (NULL == p_ser->data) {
    return false;
  }

  p_ser->count = 0;
  p_ser->capacity = SER_INIT_CAPACITY;

  return true;
}

static bool serializer_append_byte(Serializer *p_ser, char byte) {
  assert(NULL != p_ser);
  if (!serializer_data_maybe_expand(p_ser)) {
    return false;
  }

  p_ser->data[p_ser->count++] = byte;
  return true;
}

static bool serializer_append_cstr(Serializer *p_ser, const char *str) {
  assert(NULL != p_ser);
  assert(NULL != str);

  const char *iter = str;
  while (*iter != '\0') {
    SER_VALIDATE(serializer_append_byte(p_ser, *iter++));
  }
  return true;
}






// ----------------- | PUBLIC |
bool serializer_start_serialization(Serializer *p_ser, SerializationKind kind) {
  assert(NULL != p_ser);
  // TODO: serializer free
  if (!serializer_init(p_ser)) {
    return false;
  }

#ifndef NDEBUG
  p_ser->tag = kind;
#else
  (void)kind;
#endif // !NDEBUG

  return true;
}

bool serializer_end_serialization(Serializer *p_ser, SerializationKind kind) {
  assert(NULL != p_ser);
  assert(kind == p_ser->tag);
  
  switch (kind) {
    case SER_KIND_JSON: return serializer_append_byte(p_ser, '\0');
    default: return false;
  }
}

void serializer_free(Serializer *p_ser) {
  assert(NULL != p_ser);
  free(p_ser->data);
  *p_ser = (Serializer){0};
}

bool serializer_json_start_object(Serializer *p_ser) {
  assert(NULL != p_ser);
  assert(SER_KIND_JSON == p_ser->tag);
  return serializer_append_byte(p_ser, '{');
}

bool serializer_json_end_object(Serializer *p_ser) {
  assert(NULL != p_ser);
  assert(0 != p_ser->count);
  assert(SER_KIND_JSON == p_ser->tag);

  char *data_back = p_ser->data + p_ser->count - 1;
  if (',' == *data_back) {
    *data_back = '}';
    return true;
  }
  
  assert('"' == *data_back || ']' == *data_back || '}' == *data_back);
  return serializer_append_byte(p_ser, '}');
}

// bool serializer_json_start_nested_object(Serializer *p_ser, const char *name) {
//   assert(NULL != p_ser);
//   assert(SER_KIND_JSON == p_ser->tag);

//   SER_VALIDATE(serializer_json_start_field(p_ser, name));
//   SER_VALIDATE(serializer_append_byte(p_ser, '{'));
//   return true;
// }

// bool serializer_json_end_nested_object(Serializer *p_ser) {
//   assert(NULL != p_ser);
//   assert(SER_KIND_JSON == p_ser->tag);

//   SER_VALIDATE(serializer_json_end_object(p_ser));
//   return serializer_append_byte(p_ser, ',');
// }

bool serializer_json_start_array(Serializer *p_ser) {
  assert(NULL != p_ser);
  assert(SER_KIND_JSON == p_ser->tag);

  SER_VALIDATE(serializer_append_byte(p_ser, '['));
  return true;
}

bool serializer_json_end_array(Serializer *p_ser) {
  assert(NULL != p_ser);
  assert(0 != p_ser->count);
  assert(SER_KIND_JSON == p_ser->tag);

  char *data_back = p_ser->data + p_ser->count - 1;
  if (',' == *data_back) {
    *data_back = ']';
    return true;
  }

  return serializer_append_byte(p_ser, ']');
}

bool serializer_json_append_separator(Serializer *p_ser) {
  assert(NULL != p_ser);
  assert(SER_KIND_JSON == p_ser->tag);
  return serializer_append_byte(p_ser, ',');
}

void serializer_json_remove_separator_at_end(Serializer *p_ser) {
  assert(NULL != p_ser);
  assert(SER_KIND_JSON == p_ser->tag);
  p_ser->count -= ',' == p_ser->data[p_ser->count - 1];
}


bool serializer_json_start_field(Serializer *p_ser, const char *name) {
  assert(NULL != p_ser);
  assert(NULL != name);
  assert(SER_KIND_JSON == p_ser->tag);

  SER_VALIDATE(serializer_append_byte(p_ser, '"'));
  SER_VALIDATE(serializer_append_cstr(p_ser, name));
  SER_VALIDATE(serializer_append_byte(p_ser, '"'));
  return serializer_append_byte(p_ser, ':');
}

bool serializer_json_end_field(Serializer *p_ser) {
  assert(NULL != p_ser);
  assert(SER_KIND_JSON == p_ser->tag);
  return serializer_append_byte(p_ser, ',');
}


SerializerData serializer_get_data(const Serializer *p_ser) {
  assert(NULL != p_ser);
  return (SerializerData){
    .data = p_ser->data,
    .count = p_ser->count
  };
}


#define JSON_SERIALIZE_PRIMITIVE_IMPL(type, str_fmt)\
  do {\
    assert(NULL != p_ser);\
    assert(SER_KIND_JSON == p_ser->tag);\
    char buff[(CHAR_BIT * sizeof(type) - 1) / 3 + 2] = {0};\
    int n = snprintf(buff, (CHAR_BIT * sizeof(type) - 1) / 3 + 2, str_fmt, val);\
    if (n < 0) return false;\
    return serializer_append_cstr(p_ser, buff);\
  } while (0)

bool serializer_char_to_json(Serializer *p_ser, char val) {
  assert(NULL != p_ser);
  assert(SER_KIND_JSON == p_ser->tag);

  SER_VALIDATE(serializer_append_byte(p_ser, '"'));
  SER_VALIDATE(serializer_append_byte(p_ser, val));
  return serializer_append_byte(p_ser, '"');\
}

bool serializer_short_to_json(Serializer *p_ser, short val) {
  JSON_SERIALIZE_PRIMITIVE_IMPL(short, "%d");
}

bool serializer_int_to_json(Serializer *p_ser, int val) {
  JSON_SERIALIZE_PRIMITIVE_IMPL(int, "%d");
}

bool serializer_long_int_to_json(Serializer *p_ser, long int val) {
  JSON_SERIALIZE_PRIMITIVE_IMPL(long int, "%ld");
}

bool serializer_long_long_int_to_json(Serializer *p_ser, long long int val) {
  JSON_SERIALIZE_PRIMITIVE_IMPL(long long int, "%lld");
}

bool serializer_float_to_json(Serializer *p_ser, float val) {
  JSON_SERIALIZE_PRIMITIVE_IMPL(float, "%.7g");
}

bool serializer_double_to_json(Serializer *p_ser, double val) {
  JSON_SERIALIZE_PRIMITIVE_IMPL(double, "%.15g");
}

bool serializer_long_double_to_json(Serializer *p_ser, long double val) {
  assert(NULL != p_ser);\
  assert(SER_KIND_JSON == p_ser->tag);
  char buff[(CHAR_BIT * sizeof(long double) - 1) / 3 + 2] = {0};
  int n = snprintf(buff, (CHAR_BIT * sizeof(long double) - 1) / 3 + 2, "%.*Lg", LDBL_DIG, val);
  if (n < 0) return false;
  return serializer_append_cstr(p_ser, buff);
}

bool serializer_cstr_to_json(Serializer *p_ser, const char *val) {
  assert(NULL != p_ser);\
  assert(SER_KIND_JSON == p_ser->tag);
  SER_VALIDATE(serializer_append_byte(p_ser, '"'));
  SER_VALIDATE(serializer_append_cstr(p_ser, val));
  return serializer_append_byte(p_ser, '"');
}


#define JSON_SERIALIZE_PRIMITIVE_FIELD_IMPL(type, str_fmt)\
  do {\
    assert(NULL != p_ser);\
    assert(SER_KIND_JSON == p_ser->tag);\
    char buff[(CHAR_BIT * sizeof(type) - 1) / 3 + 2] = {0};\
    int n = snprintf(buff, (CHAR_BIT * sizeof(type) - 1) / 3 + 2, str_fmt, val);\
    if (n < 0) return false;\
    SER_VALIDATE(serializer_json_start_field(p_ser, name));\
    SER_VALIDATE(serializer_append_cstr(p_ser, buff));\
    return serializer_json_end_field(p_ser);\
  } while (0)

bool serializer_json_field_from_char(Serializer *p_ser, const char *name, char val) {
  assert(NULL != p_ser);
  assert(SER_KIND_JSON == p_ser->tag);

  SER_VALIDATE(serializer_json_start_field(p_ser, name));
  SER_VALIDATE(serializer_char_to_json(p_ser, val));
  return serializer_json_end_field(p_ser);
}

bool serializer_json_field_from_short(Serializer *p_ser, const char *name, short val) {
  JSON_SERIALIZE_PRIMITIVE_FIELD_IMPL(short, "%d");
}

bool serializer_json_field_from_int(Serializer *p_ser, const char *name, int val) {
  JSON_SERIALIZE_PRIMITIVE_FIELD_IMPL(int, "%d");
}

bool serializer_json_field_from_long_int(Serializer *p_ser, const char *name, long int val) {
  JSON_SERIALIZE_PRIMITIVE_FIELD_IMPL(long int, "%ld");
}

bool serializer_json_field_from_long_long_int(Serializer *p_ser, const char *name, long long int val) {
  JSON_SERIALIZE_PRIMITIVE_FIELD_IMPL(long long int, "%lld");
}

bool serializer_json_field_from_float(Serializer *p_ser, const char *name, float val) {
  JSON_SERIALIZE_PRIMITIVE_FIELD_IMPL(float, "%.7g");
}

bool serializer_json_field_from_double(Serializer *p_ser, const char *name, double val) {
  JSON_SERIALIZE_PRIMITIVE_FIELD_IMPL(double, "%.15g");
}

bool serializer_json_field_from_long_double(Serializer *p_ser, const char *name, long double val) {
  assert(NULL != p_ser);\
  assert(SER_KIND_JSON == p_ser->tag);
  char buff[(CHAR_BIT * sizeof(long double) - 1) / 3 + 2] = {0};
  int n = snprintf(buff, (CHAR_BIT * sizeof(long double) - 1) / 3 + 2, "%.*Lg", LDBL_DIG, val);
  if (n < 0) return false;
  SER_VALIDATE(serializer_json_start_field(p_ser, name));
  SER_VALIDATE(serializer_append_cstr(p_ser, buff));
  return serializer_json_end_field(p_ser);
}

bool serializer_json_field_from_cstr(Serializer *p_ser, const char *name, const char *val) {
  assert(NULL != p_ser);\
  assert(SER_KIND_JSON == p_ser->tag);
  SER_VALIDATE(serializer_json_start_field(p_ser, name));
  SER_VALIDATE(serializer_append_byte(p_ser, '"'));
  SER_VALIDATE(serializer_append_cstr(p_ser, val));
  SER_VALIDATE(serializer_append_byte(p_ser, '"'));
  return serializer_json_end_field(p_ser);
}



