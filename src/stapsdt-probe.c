#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

#include "libstapsdt.h"
#define _SDT_HAS_SEMAPHORES 1

#include "sdt.h"

#define SEC(name) __attribute__((section(name), used))

/* USDT probe firing is used as a mechanism to support system-wide
 * tracing of libstapsdt probe firings; we define provider/probe
 * stapsdt/probe and pass it the provider and probe names along
 * with up to 6 arguments.  Tracers can then attach to libstapsdt.so
 * and catch dynamic probe firings system-wide.  For example,
 * in a libbpf-based program:
 *
 * SEC("usdt/libstapsdt.so::stapsdt:probe")
 * int BPF_USDT(myprobe, char *provider, char *probe)
 * {
 *   __bpf_printk("%s/%s fired!\n"); 
 * }
 *
 * To support is-enabled functionality, we specify an associated
 * semaphore with the probe; its count is bumped up for each
 * tracer attached.
 */
unsigned short stapsdt_probe_semaphore SEC(".probes");

void stapsdtProbeFire(SDTProbe_t *probe, uint64_t *args)
{
   STAP_PROBE8(stapsdt, probe, probe->provider->name, probe->name,
               args[0], args[1], args[2], args[3], args[4], args[5]);
}

int stapsdtProbeIsEnabled(void)
{
	return stapsdt_probe_semaphore > 0;
}
