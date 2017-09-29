#include <stdio.h>
#include <unistd.h>
#include <libstapsdt.h>

int main( int argc, char *argv[] ) {
  SDTProvider_t *provider;
  SDTProbe_t *probe;

  if(argc != 3) {
    printf("usage: demo PROVIDER PROBE\n");
    return -1;
  }

  provider = providerInit(argv[1]);
  probe = providerAddProbe(provider, argv[2]);

  if(providerLoad(provider) == -1) {
    printf("Something went wrong...\n");
    return -1;
  }

  while(1) {
    printf("Firing probe...\n");
    probeFire(probe);
    printf("Probe fired!\n");
    sleep(3);
  }

  return 0;
}
