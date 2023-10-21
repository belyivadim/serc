//typedef void (*fptr)(int param);
typedef int bool;

struct Test {
  int i;
  float f;
  const double long dl;
  const char * const * const **pc;
  const long long int *plli;
  //fptr f;
};

struct Test2 {
  struct Test t;
};

int main() {
  return 0x0l;
}

// class LoxClass {
//   foo(a, b) {
//     return a + b;
//   }
// }

// var lc = LoxClass()
// print lc.foo()
