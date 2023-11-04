//typedef void (*fptr)(int param);
typedef int bool;
typedef const int ID;

typedef struct Test {
  ID ****ids;
  int i;
  float f;
  const double long dl;
  bool b;
  //const char * const * const **pc;
  //const long long int *plli;
  //fptr f;
} Test;

struct Test2 {
  Test * arr; // `@array @size arr_count`
  unsigned int arr_count; // `@omit`
};

int main() {
  return 0x0l;
}

