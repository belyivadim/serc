//typedef void (*fptr)(int param);
typedef int bool;

typedef struct Test {
  int i;
  float f;
  const double long dl;
  bool b;
  //const char * const * const **pc;
  //const long long int *plli;
  //fptr f;
} Test;

struct Test2 {
  Test t;
};

int main() {
  return 0x0l;
}

