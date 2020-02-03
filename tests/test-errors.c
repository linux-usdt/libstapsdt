#include "libstapsdt.h"
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
#include <sys/syscall.h>
#ifdef __NR_memfd_create // older glibc may not have this syscall defined
#define HAVE_LIBSTAPSDT_MEMORY_BACKED_FD
#endif
#endif

int testElfCreationError() {
  // TODO (mmarchini) write test case for elf creation error
  return 1;
}

int testTmpCreationError() {
  SDTProvider_t *provider;
  SDTError_t errno;
#ifdef HAVE_LIBSTAPSDT_MEMORY_BACKED_FD
  char *providerLongName = "test/probe/creation/error/with/a/name/longer/than/"
                           "249/characters"
                           "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                           "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                           "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                           "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

  provider = providerInit(providerLongName);
#else
  provider = providerInit("test/probe/creation/error");
#endif
  if(providerLoad(provider) == 0) {
    return 0;
  }
  printf("[testTmpCreationError] Error message: %s\n", provider->error);
  errno = provider->errno;
  providerDestroy(provider);
  return errno == tmpCreationError;
}

int testSharedLibraryOpenError() {
  // TODO (mmarchini) write test case for shared library loading error
  return 1;
}

int testSymbolLoadingError() {
  // TODO (mmarchini) write test case for symbol loading error
  return 1;
}

int testSharedLibraryCloseError() {
  SDTProvider_t *provider;
  SDTError_t errno;
  provider = providerInit("test-error");
  providerLoad(provider);
  dlclose(provider->_handle);
  if(providerUnload(provider) == 0) {
    return 0;
  }
  errno = provider->errno;
  printf("[testSharedLibraryCloseError] Error message: %s\n", provider->error);
  providerDestroy(provider);
  return errno == sharedLibraryCloseError;
}

int main() {
  if (!testElfCreationError()) {
    printf("Test case failed: testElfCreationError\n");
    return -1;
  }

  if (!testTmpCreationError()) {
    printf("Test case failed: testTmpCreationError\n");
    return -2;
  }

  if (!testSharedLibraryOpenError()) {
    printf("Test case failed: testSharedLibraryOpenError\n");
    return -3;
  }

  if (!testSymbolLoadingError()) {
    printf("Test case failed: testSymbolLoadingError\n");
    return -4;
  }

  if (!testSharedLibraryCloseError()) {
    printf("Test case failed: testSharedLibraryCloseError\n");
    return -5;
  }


  return 0;
}
