#ifndef __SERC_SERIALIZATION_PRIMITIVES_H__
#define __SERC_SERIALIZATION_PRIMITIVES_H__

#include <stdbool.h>
#include <stddef.h>

typedef enum {
  SER_KIND_UNINITIALIZED = 0,
  SER_KIND_JSON
} SerializationKind;

typedef struct {
  const char *data;
  size_t count;
} SerializerData;

typedef struct {
  char *data;
  size_t count;
  size_t capacity;
#ifndef NDEBUG
  SerializationKind tag;
#endif // !NDEBUG
} Serializer;

bool serializer_start_serialization(Serializer *p_ser, SerializationKind kind);
bool serializer_end_serialization(Serializer *p_ser, SerializationKind kind);
void serializer_free(Serializer *p_ser);

bool serializer_json_start_object(Serializer *p_ser);
bool serializer_json_end_object(Serializer *p_ser);

// bool serializer_json_start_nested_object(Serializer *p_ser, const char *name);
// bool serializer_json_end_nested_object(Serializer *p_ser);
bool serializer_json_start_array(Serializer *p_ser);
bool serializer_json_end_array(Serializer *p_ser);
bool serializer_json_start_field(Serializer *p_ser, const char *name);
bool serializer_json_end_field(Serializer *p_ser);
bool serializer_json_append_separator(Serializer *p_ser);
void serializer_json_remove_separator_at_end(Serializer *p_ser);

bool serializer_char_to_json(Serializer *p_ser, char val);
bool serializer_short_to_json(Serializer *p_ser, short val);
bool serializer_int_to_json(Serializer *p_ser, int val);
bool serializer_long_int_to_json(Serializer *p_ser, long int val);
bool serializer_long_long_int_to_json(Serializer *p_ser, long long int val);
bool serializer_float_to_json(Serializer *p_ser, float val);
bool serializer_double_to_json(Serializer *p_ser, double val);
bool serializer_long_double_to_json(Serializer *p_ser, long double val);
bool serializer_cstr_to_json(Serializer *p_ser, const char *val);

// TODO:
bool serializer_u_char_to_json(Serializer *p_ser, char val);
bool serializer_u_short_to_json(Serializer *p_ser, short val);
bool serializer_u_int_to_json(Serializer *p_ser, int val);
bool serializer_u_long_int_to_json(Serializer *p_ser, long int val);
bool serializer_u_long_long_int_to_json(Serializer *p_ser, long long int val);



bool serializer_json_field_from_char(Serializer *p_ser, const char *name, char val);
bool serializer_json_field_from_short(Serializer *p_ser, const char *name, short val);
bool serializer_json_field_from_int(Serializer *p_ser, const char *name, int val);
bool serializer_json_field_from_long_int(Serializer *p_ser, const char *name, long int val);
bool serializer_json_field_from_long_long_int(Serializer *p_ser, const char *name, long long int val);
bool serializer_json_field_from_float(Serializer *p_ser, const char *name, float val);
bool serializer_json_field_from_double(Serializer *p_ser, const char *name, double val);
bool serializer_json_field_from_long_double(Serializer *p_ser, const char *name, long double val);
bool serializer_json_field_from_cstr(Serializer *p_ser, const char *name, const char *val);


SerializerData serializer_get_data(const Serializer *p_ser);


#endif // !__SERC_SERIALIZATION_PRIMITIVES_H__
