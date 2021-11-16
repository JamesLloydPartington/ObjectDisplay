#include <iostream>

using namespace std;


int main(void)
{
  int a = 10;
  int &b = a;
  printf("%d %d", a, b);
  return 0;
}
