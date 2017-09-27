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

// ------------------------------------------------------------------------- //
// TODO(matheus): dynamic strings creation

int createSharedLibrary(int fd, char *provider, char *probe) {
  // Dynamic strings
  DynElf *dynElf = dynElfInit(fd);

  dynElfAddProbe(dynElf, provider, probe);

  if(dynElfSave(dynElf) == -1) {
    return -1;
  }

  (void)elf_end(dynElf->elf);

  /* Finished */
  return 0;
}

void *registerProbe(char *provider, char *probe) {
  int fd;
  void *handle;
  void *fireProbe;
  char filename[sizeof("/tmp/") + sizeof(provider) + sizeof(probe) +
                sizeof("XXXXXX") + 2];
  char *error;

  sprintf(filename, "/tmp/%s-%s-XXXXXX", provider, probe);

  if ((fd = mkstemp(filename)) < 0) {
    return NULL;
  }

  createSharedLibrary(fd, provider, probe);
  handle = dlopen(filename, RTLD_LAZY);
  if (!handle) {
    fputs(dlerror(), stderr);
    return NULL;
  }

  fireProbe = dlsym(handle, PROBE_SYMBOL);

  if ((error = dlerror()) != NULL) {
    fputs(error, stderr);
    return NULL;
  }

  (void)close(fd);
  return fireProbe;
}
