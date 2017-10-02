#include <stdarg.h>
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

int createSharedLibrary(int fd, SDTProvider_t *provider) {
  DynElf *dynElf = dynElfInit(fd);

  for(SDTProbeList_t *node=provider->probes; node != NULL; node = node->next) {
    dynElfAddProbe(dynElf, &(node->probe));
  }

  if(dynElfSave(dynElf) == -1) {
    return -1;
  }

  dynElfClose(dynElf);

  return 0;
}

SDTProvider_t *providerInit(char *name) {
  SDTProvider_t *provider = (SDTProvider_t *) calloc(sizeof(SDTProvider_t), 1);
  provider->probes = NULL;

  provider->name = (char *) calloc(sizeof(char), strlen(name) + 1);
  memcpy(provider->name, name, sizeof(char) * strlen(name) + 1);

  return provider;
}

SDTProbe_t *providerAddProbe(SDTProvider_t *provider, char *name, int argCount, ...) {
  int i;
  va_list vl;
  ArgType_t arg;
  va_start(vl, argCount);

  SDTProbeList_t *probeList = (SDTProbeList_t *) calloc(sizeof(SDTProbeList_t), 1);
  probeList->probe._fire = NULL;

  probeList->probe.name = (char *) calloc(sizeof(char), strlen(name) + 1);
  memcpy(probeList->probe.name, name, sizeof(char) * strlen(name) + 1);

  probeList->next = provider->probes;
  provider->probes = probeList;

  probeList->probe.argCount = argCount;

  for(i=0; i < argCount; i++) {
    arg = va_arg(vl, ArgType_t);
    probeList->probe.argFmt[i] = arg;
  }

  for(; i<MAX_ARGUMENTS; i++) {
    probeList->probe.argFmt[i] = noarg;
  }

  probeList->probe.provider = provider;

  return &(probeList->probe);
}

int providerLoad(SDTProvider_t *provider) {
  int fd;
  void *handle;
  void *fireProbe;
  char filename[sizeof("/tmp/") + sizeof(provider->name) + sizeof("XXXXXX") + 1];
  char *error;

  sprintf(filename, "/tmp/%s-XXXXXX", provider->name);

  if ((fd = mkstemp(filename)) < 0) {
    printf("Couldn't create '%s'\n", filename);
    return -1;
  }

  createSharedLibrary(fd, provider);
  (void)close(fd);

  handle = dlopen(filename, RTLD_LAZY);
  if (!handle) {
    fputs(dlerror(), stderr);
    return -1;
  }

  for(SDTProbeList_t *node=provider->probes; node != NULL; node = node->next) {
    fireProbe = dlsym(handle, node->probe.name);
    node->probe._fire = fireProbe;
  }

  if ((error = dlerror()) != NULL) {
    fputs(error, stderr);
    return -1;
  }

  return 0;
}

void probeFire(SDTProbe_t *probe, ...) {
  va_list vl;
  va_start(vl, probe);
  uint64_t arg[6] = {0};
  for(int i=0; i < probe->argCount; i++) {
    arg[i] = va_arg(vl, uint64_t);
  }

  switch(probe->argCount) {
    case 0:
      ((void (*)())probe->_fire) ();
      return;
    case 1:
      ((void (*)())probe->_fire) (arg[0]);
      return;
    case 2:
      ((void (*)())probe->_fire) (arg[0], arg[1]);
      return;
    case 3:
      ((void (*)())probe->_fire) (arg[0], arg[1], arg[2]);
      return;
    case 4:
      ((void (*)())probe->_fire) (arg[0], arg[1], arg[2], arg[3]);
      return;
    case 5:
      ((void (*)())probe->_fire) (arg[0], arg[1], arg[2], arg[3], arg[4]);
      return;
    case 6:
      ((void (*)())probe->_fire) (arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
      return;
    default:
      ((void (*)())probe->_fire) ();
      return;
  }
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
