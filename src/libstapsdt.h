
typedef struct  {
  char *name;
  void *_fire;
} SDTProbe_t;

typedef struct SDTProbeList_ {
  SDTProbe_t probe;
  struct SDTProbeList_ *next;
} SDTProbeList_t;

typedef struct SDTProvider {
  char *name;
  SDTProbeList_t *probes;
} SDTProvider_t;

SDTProvider_t *providerInit(char *name);

SDTProbe_t *providerAddProbe(SDTProvider_t *provider, char *name);

int providerLoad(SDTProvider_t *provider);

void providerDestroy(SDTProvider_t *provider);

void probeFire(SDTProbe_t *probe);
