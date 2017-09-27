#ifndef _SHARED_LIB_H
#define _SHARED_LIB_H

#include "section.h"
#include "string-table.h"

typedef struct {
  Elf *elf;
  Elf64_Ehdr *ehdr;
  Elf64_Phdr *phdrLoad1, *phdrLoad2, *phdrDyn;

  StringTable *stringTable;
  StringTable *dynamicString;

  Section *sectionsList[9];
  struct {
    Section
      *hash,
      *dynSym,
      *dynStr,
      *text,
      *sdtBase,
      *ehFrame,
      *dynamic,
      *sdtNote,
      *shStrTab;
  } sections;
} DynElf;

DynElf *dynElfInit();

#endif
