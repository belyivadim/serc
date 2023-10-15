#include <stdio.h>

#include "primitives.h"

int main() {
  Serializer ser;
  serializer_start_serialization(&ser, SER_KIND_JSON);

  serializer_json_start_array(&ser);
  serializer_json_start_object(&ser);
  {
    serializer_json_field_from_short(&ser, "rate", 32000);
    serializer_json_field_from_char(&ser, "category", 'A');
    bool ok = serializer_json_field_from_float(&ser, "float", 5.123456789);
    if (!ok) printf("Could not serialize float\n");

    ok = serializer_json_field_from_long_double(&ser, "ld", 12345.12345678901234567890L);
    if (!ok) printf("Could not serialize long double\n");

    serializer_json_start_field(&ser, "llis");
    serializer_json_start_array(&ser);
    for (long long int i = 0; i < 3; ++i) {
      serializer_long_long_int_to_json(&ser, i);
      serializer_json_append_separator(&ser);
    }
    serializer_json_remove_separator_at_end(&ser);
    serializer_json_end_field(&ser);
    serializer_json_end_array(&ser);
    serializer_json_append_separator(&ser);

    serializer_json_start_field(&ser, "chars");
    serializer_json_start_array(&ser);
    for (int i = 0; i < 3; ++i) {
      serializer_char_to_json(&ser, i + 'a');
      serializer_json_append_separator(&ser);
    }
    serializer_json_remove_separator_at_end(&ser);
    serializer_json_end_field(&ser);
    serializer_json_end_array(&ser);
    serializer_json_append_separator(&ser);



    serializer_json_start_field(&ser, "users");
    serializer_json_start_array(&ser);
    {
      serializer_json_start_object(&ser);
      {
        serializer_json_field_from_cstr(&ser, "username", "user1");
        serializer_json_field_from_cstr(&ser, "email", "user1@mail.com");
        serializer_json_field_from_cstr(&ser, "password", "password1");

        serializer_json_start_field(&ser, "balance");
        serializer_json_start_object(&ser);
        {
          serializer_json_field_from_int(&ser, "money", 1231231231);
        }
        serializer_json_end_object(&ser);
        serializer_json_end_field(&ser);
      }
      serializer_json_end_object(&ser);
      serializer_json_append_separator(&ser);

      serializer_json_start_object(&ser);
      {
        serializer_json_field_from_cstr(&ser, "username", "user1");
        serializer_json_field_from_cstr(&ser, "email", "user1@mail.com");
        serializer_json_field_from_cstr(&ser, "password", "password1");

        serializer_json_start_field(&ser, "balance");
        serializer_json_start_object(&ser);
        {
          serializer_json_field_from_int(&ser, "money", 1231231231);
        }
        serializer_json_end_object(&ser);
        serializer_json_end_field(&ser);
      }
      serializer_json_end_object(&ser);
    }
    serializer_json_end_array(&ser);
    serializer_json_end_field(&ser);
  }
  serializer_json_end_object(&ser);
  serializer_json_end_array(&ser);


  serializer_end_serialization(&ser, SER_KIND_JSON);

  SerializerData sd = serializer_get_data(&ser);
  printf("%s\n", sd.data);

  serializer_free(&ser);

  return 0;
}
