#include <stdio.h>
#include <unistd.h>
#include <libstapsdt.h>

int main( int argc, char *argv[] ) {
  SDTProvider_t *provider;
  SDTProbe_t *probe1, *probe2;

  provider = providerInit("testProvider");
  probe1 = providerAddProbe(provider, "testProbe1", 4, int8, uint8, int64, uint64);
  probe2 = providerAddProbe(provider, "testProbe2", 2, int8, uint8);

  if(providerLoad(provider) == -1) {
    printf("Something went wrong...\n");
    return -1;
  }

  probeFire(probe1, 1, 2, 3, 4);
  probeFire(probe2, -3, 8);

  providerUnload(provider);
  providerDestroy(provider);

  return 0;
}
