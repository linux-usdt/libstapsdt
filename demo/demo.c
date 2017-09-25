#include <stdio.h>
#include <unistd.h>
#include <libstapsdt.h>

int main( int argc, char *argv[] ) {
  int (*fireProbe)();

  if(argc != 3) {
    printf("usage: demo PROVIDER PROBE\n");
    return -1;
  }

  fireProbe = registerProbe(argv[1], argv[2]);

  while(1) {
    printf("Firing probe...\n");
    fireProbe();
    printf("Probe fired!\n");
    sleep(3);
  }

  return 0;
}
