#ifndef _SHARED_LIB_H
#define _SHARED_LIB_H

#include "section.h"
#include "string-table.h"
#include "sdtnote.h"
#include "dynamic-symbols.h"

typedef struct {
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
} SectionsList;

typedef struct {
  Elf *elf;
  Elf64_Ehdr *ehdr;
  Elf64_Phdr *phdrLoad1, *phdrLoad2, *phdrDyn;

  StringTable *stringTable;
  StringTable *dynamicString;

  DynamicSymbolTable *dynamicSymbols;

  SDTNoteList_t *sdtNotes;
  size_t sdtNotesCount;

  SectionsList sections;
} DynElf;

DynElf *dynElfInit();

int dynElfAddProbe(DynElf *dynElf, SDTProbe_t *probe);

int dynElfSave(DynElf *dynElf);

void dynElfClose(DynElf *dynElf);

#endif
