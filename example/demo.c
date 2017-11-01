#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libstapsdt.h>

int main( int argc, char *argv[] ) {
  SDTProvider_t *provider;
  SDTProbe_t **probes;
  int probesCount = 0;
  unsigned long long i=0;
  int j=0;

  if(argc < 3) {
    printf("usage: demo PROVIDER PROBE\n");
    return -1;
  }

  probesCount = argc - 2;
  probes = calloc(sizeof(SDTProvider_t *), probesCount);

  provider = providerInit(argv[1]);
  for (int idx = 0; idx < (probesCount); idx++) {
    probes[idx] = providerAddProbe(provider, argv[idx + 2], 2, uint64, int64);
  }

  if(providerLoad(provider) == -1) {
    printf("Something went wrong: %s\n", provider->error);
    return -1;
  }

  while(1) {
    printf("Firing probes...\n");
    for (int idx = 0; idx < probesCount; idx++) {
      printf("Firing probe [%d]...\n", idx);
      probeFire(probes[idx], i++, j--);
    }
    printf("Probe fired!\n");
    sleep(3);
  }

  return 0;
}
