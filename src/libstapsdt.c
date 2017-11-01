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
#include "errors.h"

int createSharedLibrary(int fd, SDTProvider_t *provider) {
  DynElf *dynElf = dynElfInit(fd);

  for(SDTProbeList_t *node=provider->probes; node != NULL; node = node->next) {
    dynElfAddProbe(dynElf, &(node->probe));
  }

  if(dynElfSave(dynElf) == -1) {
    sdtSetError(provider, elfCreationError, provider->name);
    return -1;
  }

  dynElfClose(dynElf);

  return 0;
}

SDTProvider_t *providerInit(const char *name) {
  SDTProvider_t *provider = (SDTProvider_t *) calloc(sizeof(SDTProvider_t), 1);
  provider->error     = NULL;
  provider->errno     = noError;
  provider->probes    = NULL;
  provider->_handle   = NULL;
  provider->_filename = NULL;

  provider->name = (char *) calloc(sizeof(char), strlen(name) + 1);
  memcpy(provider->name, name, sizeof(char) * strlen(name) + 1);

  return provider;
}

SDTProbe_t *providerAddProbe(SDTProvider_t *provider, const char *name, int argCount, ...) {
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
  void *fireProbe;
  char *filename = calloc(sizeof(char), strlen("/tmp/-XXXXXX.so") + strlen(provider->name) + 1);
  char *error;

  sprintf(filename, "/tmp/%s-XXXXXX.so", provider->name);

  if ((fd = mkstemps(filename, 3)) < 0) {
    sdtSetError(provider, tmpCreationError, filename);
    free(filename);
    return -1;
  }
  provider->_filename = filename;

  if(createSharedLibrary(fd, provider) != 0) {
    (void)close(fd);
    return -1;
  }
  (void)close(fd);

  provider->_handle = dlopen(filename, RTLD_LAZY);
  if (!provider->_handle) {
    sdtSetError(provider, sharedLibraryOpenError, filename, dlerror());
    return -1;
  }

  for(SDTProbeList_t *node=provider->probes; node != NULL; node = node->next) {
    fireProbe = dlsym(provider->_handle, node->probe.name);

    // TODO (mmarchini) handle errors better when a symbol fails to load
    if ((error = dlerror()) != NULL) {
      sdtSetError(provider, sharedLibraryOpenError, filename, node->probe.name, error);
      return -1;
    }

    node->probe._fire = fireProbe;
  }


  return 0;
}

int providerUnload(SDTProvider_t *provider) {
  if(provider->_handle == NULL) {
    return 0;
  }
  if(dlclose(provider->_handle) != 0) {
    sdtSetError(provider, sharedLibraryCloseError, provider->_filename, provider->name, dlerror());
    return -1;
  }
  provider->_handle = NULL;

  for(SDTProbeList_t *node=provider->probes; node != NULL; node = node->next) {
    node->probe._fire = NULL;
  }

  unlink(provider->_filename);
  free(provider->_filename);

  return 0;
}

void probeFire(SDTProbe_t *probe, ...) {
  if(probe->_fire == NULL) {
    return;
  }
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

int probeIsEnabled(SDTProbe_t *probe) {
  if(probe->_fire == NULL) {
    return 0;
  }
  if(((*(char *)probe->_fire) & 0x90) == 0x90) {
    return 0;
  }
  return 1;
}

void providerDestroy(SDTProvider_t *provider) {
  SDTProbeList_t *node=NULL, *next=NULL;

  for(node=provider->probes; node!=NULL; node=next) {
    free(node->probe.name);
    next=node->next;
    free(node);
  }
  free(provider->name);
  if(provider->error != NULL) {
    free(provider->error);
  }
  free(provider);
}
