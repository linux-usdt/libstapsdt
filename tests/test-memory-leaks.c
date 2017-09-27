#include <stdio.h>
#include <unistd.h>
#include <libstapsdt.h>

int main( int argc, char *argv[] ) {
  int (*fireProbe)();

  fireProbe = registerProbe("test-provider", "test-probe");

  if(fireProbe == NULL)
    return -1;

  fireProbe();

  return 0;
}
