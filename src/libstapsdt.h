#ifndef _LIBSTAPSDT_H
#define _LIBSTAPSDT_H
#define MAX_ARGUMENTS 6

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

typedef struct  {
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
} SDTProvider_t;

SDTProvider_t *providerInit(const char *name);

SDTProbe_t *providerAddProbe(SDTProvider_t *provider, const char *name, int argCount, ...);

int providerLoad(SDTProvider_t *provider);

void providerDestroy(SDTProvider_t *provider);

void probeFire(SDTProbe_t *probe, ...);

#endif
