#include <stdio.h>

#include "../src/serialization/primitives.h"
#include "../src/serialization/json.h"

//typedef void (*fptr)(int param);
typedef const int ID;

typedef struct Test {
  ID ****ids;
  int i;
  float f;
  const double long dl;
  //bool b;
  //const char * const * const **pc;
  //const long long int *plli;
  //fptr f;
} Test;

struct Test2 {
  Test * arr; // `@array @size arr_count`
  unsigned int arr_count; // `@omit`
  void *v; // `@callback @s cb_void_to_json @d cb_json_to_void`
};

bool cb_void_to_json(Serializer *p_ser, const struct Test2 *v) {
  (void)p_ser;
  printf("%p\n", (void*)v);
  return true;
}

bool cb_json_to_void(Serializer *p_ser, const void *v) {
  (void)p_ser;
  (void)v;
  return true;
}


int main() {
  Serializer ser;
  serializer_start_serialization(&ser, SER_KIND_JSON);

  struct Test2 t2 = {.arr_count = 0, .v = (void*)0xDEADBEEF};

  serializer_Test2_to_json(&ser, &t2);

  serializer_end_serialization(&ser, SER_KIND_JSON);
  serializer_free(&ser);
  return 0x0l;
}

