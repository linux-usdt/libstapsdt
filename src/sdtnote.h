#ifndef _SDT_NOTE_H
#define _SDT_NOTE_H

#include <libelf.h>
#include "libstapsdt.h"

#define NT_STAPSDT 3
#define NT_STAPSDT_NAME "stapsdt"

typedef struct SDTNote_ {
  // Header
  Elf64_Nhdr header;
  // Note name
  char *name;
  struct {
    // Note description
    Elf64_Xword probePC;
    Elf64_Xword base_addr;
    Elf64_Xword sem_addr;
    char *provider; // mainer
    char *probe;    //
    char *argFmt;   // \0
  } content;
} SDTNote;

size_t sdtNoteSize(SDTNote *sdt);

SDTNote *sdtNoteInit(SDTProbe_t *probe);

int sdtNoteToBuffer(SDTNote *sdt, char *buffer);

void sdtNoteFree(SDTNote *sdtNote);

#endif
