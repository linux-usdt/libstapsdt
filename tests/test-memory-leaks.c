#include <stdio.h>
#include <unistd.h>
#include <libstapsdt.h>

int main( int argc, char *argv[] ) {
  SDTProvider_t *provider;
  SDTProbe_t *probe;

  provider = providerInit("testProvider");
  probe = providerAddProbe(provider, "testProbe");

  if(providerLoad(provider) == -1) {
    printf("Something went wrong...\n");
    return -1;
  }

  probeFire(probe);

  providerDestroy(provider);

  return 0;
}
