#include <dlfcn.h>
#include <err.h>
#include <fcntl.h>
#include <libelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "dynamic-symbols.h"
#include "sdtnote.h"
#include "section.h"
#include "string-table.h"
#include "shared-lib.h"
#include "util.h"
#include "libstapsdt.h"

// ------------------------------------------------------------------------- //
// TODO(mmarchini): dynamic strings creation

int createSharedLibrary(int fd, char *provider, char *probe) {
  // Dynamic strings
  DynElf *dynElf = dynElfInit(fd);

  dynElfAddProbe(dynElf, provider, probe);

  if(dynElfSave(dynElf) == -1) {
    return -1;
  }

  dynElfClose(dynElf);

  /* Finished */
  return 0;
}

SDTProvider_t *providerInit(char *name) {
  SDTProvider_t *provider = (SDTProvider_t *) calloc(sizeof(SDTProvider_t), 1);
  provider->probes = NULL;

  provider->name = (char *) calloc(sizeof(char), strlen(name) + 1);
  memcpy(provider->name, name, sizeof(char) * strlen(name) + 1);

  return provider;
}

SDTProbe_t *providerAddProbe(SDTProvider_t *provider, char *name) {
  SDTProbeList_t *probeList = (SDTProbeList_t *) calloc(sizeof(SDTProbeList_t), 1);
  probeList->probe._fire = NULL;

  probeList->probe.name = (char *) calloc(sizeof(char), strlen(name) + 1);
  memcpy(probeList->probe.name, name, sizeof(char) * strlen(name) + 1);

  probeList->next = provider->probes;
  provider->probes = probeList;

  return &(probeList->probe);
}

int providerLoad(SDTProvider_t *provider) {
  // TODO (mmarchini) multiple probes, better code to handle that
  SDTProbe_t *probe = &(provider->probes->probe);

  int fd;
  void *handle;
  void *fireProbe;
  char filename[sizeof("/tmp/") + sizeof(provider->name) + sizeof(probe->name) +
                sizeof("XXXXXX") + 2];
  char *error;

  sprintf(filename, "/tmp/%s-%s-XXXXXX", provider->name, probe->name);

  if ((fd = mkstemp(filename)) < 0) {
    printf("Couldn't create '%s'\n", filename);
    return -1;
  }

  createSharedLibrary(fd, provider->name, probe->name);
  (void)close(fd);

  handle = dlopen(filename, RTLD_LAZY);
  if (!handle) {
    fputs(dlerror(), stderr);
    return -1;
  }

  fireProbe = dlsym(handle, probe->name);
  probe->_fire = fireProbe;

  if ((error = dlerror()) != NULL) {
    fputs(error, stderr);
    return -1;
  }

  return 0;
}

void probeFire(SDTProbe_t *probe) {
  ((void (*)())probe->_fire) ();
}

void providerDestroy(SDTProvider_t *provider) {
  SDTProbeList_t *node=NULL, *next=NULL;
  for(node=provider->probes; node!=NULL; node=next) {
    free(node->probe.name);
    next=node->next;
    free(node);
  }
  free(provider->name);
  free(provider);
}
