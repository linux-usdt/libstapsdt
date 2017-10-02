#include <stdio.h>
#include <unistd.h>
#include <libstapsdt.h>

int main( int argc, char *argv[] ) {
  SDTProvider_t *provider;
  SDTProbe_t *probe;

  provider = providerInit("testProvider");
  probe = providerAddProbe(provider, "testProbe", 4, int8, uint8, int64, uint64);

  if(providerLoad(provider) == -1) {
    printf("Something went wrong...\n");
    return -1;
  }

  probeFire(probe, 1, 2, 3, 4);

  providerDestroy(provider);

  return 0;
}
