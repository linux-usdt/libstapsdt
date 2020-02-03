#ifndef _LIBSTAPSDT_H
#define _LIBSTAPSDT_H
#define MAX_ARGUMENTS 6

typedef enum {
  noError                 = -1,
  elfCreationError        = 0,
  tmpCreationError        = 1,
  sharedLibraryOpenError  = 2,
  symbolLoadingError      = 3,
  sharedLibraryCloseError = 4,
} SDTError_t;

typedef enum {
  noarg = 0,
  uint8 = 1,
  int8 = -1,
  uint16 = 2,
  int16 = -2,
  uint32 = 4,
  int32 = -4,
  uint64 = 8,
  int64 = -8,
} ArgType_t;

struct SDTProvider;

typedef struct SDTProbe {
  char *name;
  ArgType_t argFmt[MAX_ARGUMENTS];
  void *_fire;
  struct SDTProvider *provider;
  int argCount;
} SDTProbe_t;

typedef struct SDTProbeList_ {
  SDTProbe_t probe;
  struct SDTProbeList_ *next;
} SDTProbeList_t;

typedef struct SDTProvider {
  char *name;
  SDTProbeList_t *probes;
  SDTError_t errno;
  char *error;

  // private
  void *_handle;
  char *_filename;
  int _memfd;
  int _use_memfd;
} SDTProvider_t;

SDTProvider_t *providerInit(const char *name);

/*
Linux newer than 3.17 with libc that supports the syscall it will default to
using a memory-backed file descriptor. This behavior can be overridden at
runtime by calling this with use_memfd = 0 prior after providerInit, and before
providerLoad.
*/
int providerUseMemfd(SDTProvider_t *provider, const int use_memfd);

SDTProbe_t *providerAddProbe(SDTProvider_t *provider, const char *name, int argCount, ...);

int providerLoad(SDTProvider_t *provider);

int providerUnload(SDTProvider_t *provider);

void providerDestroy(SDTProvider_t *provider);

void probeFire(SDTProbe_t *probe, ...);

int probeIsEnabled(SDTProbe_t *probe);

#endif
