#include "shared-lib.h"

Elf *createElf(int fd) {
  Elf *e;

  if (elf_version(EV_CURRENT) == EV_NONE) {
    // TODO (mmarchini) error message
    return NULL;
  }

  if ((e = elf_begin(fd, ELF_C_WRITE, NULL)) == NULL) {
    // TODO (mmarchini) error message
    return NULL;
  }

  return e;
}

 int createElfStringTables(DynElf *dynElf) {
   // FIXME (mmarchini) error handling
   dynElf->stringTable = stringTableInit();
   dynElf->dynamicString = stringTableInit();

   return 0;
}

Elf64_Ehdr *createElfHeader(Elf *elf) {
  Elf64_Ehdr *ehdr;
  if ((ehdr = elf64_newehdr(elf)) == NULL) {
    // FIXME (mmarchini) properly free everything
    return NULL;
  }

  ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
  ehdr->e_type = ET_DYN;
  ehdr->e_machine = EM_X86_64;
  ehdr->e_version = EV_CURRENT;
  ehdr->e_flags = 0;

  return ehdr;
}

int createElfProgramHeaders(DynElf *dynElf) {
  if ((dynElf->phdrLoad1 = elf64_newphdr(dynElf->elf, 3)) == NULL) {
    // TODO (mmarchini) error message
    // FIXME (mmarchini) properly free everything
    return -1;
  }

  dynElf->phdrDyn = &dynElf->phdrLoad1[2];
  dynElf->phdrLoad2 = &dynElf->phdrLoad1[1];

  return 0;
}

int createElfSections(DynElf *dynElf) {
  // FIXME (mmarchini) error message
  dynElf->sections.shStrTab = sectionInit(dynElf->elf, dynElf->stringTable, ".shstrtab");
  dynElf->sections.hash = sectionInit(dynElf->elf, dynElf->stringTable, ".hash");
  dynElf->sections.dynSym = sectionInit(dynElf->elf, dynElf->stringTable, ".dynsym");
  dynElf->sections.dynStr = sectionInit(dynElf->elf, dynElf->stringTable, ".dynstr");
  dynElf->sections.text = sectionInit(dynElf->elf, dynElf->stringTable, ".text");
  dynElf->sections.ehFrame = sectionInit(dynElf->elf, dynElf->stringTable, ".eh_frame");
  dynElf->sections.dynamic = sectionInit(dynElf->elf, dynElf->stringTable, ".dynamic");
  dynElf->sections.sdtBase = sectionInit(dynElf->elf, dynElf->stringTable, ".stapsdt.base");
  dynElf->sections.sdtNote = sectionInit(dynElf->elf, dynElf->stringTable, ".note.stapsdt");

  return 0;
}

DynElf *dynElfInit(int fd) {
  DynElf *dynElf = (DynElf *)malloc(sizeof(DynElf));

  if(createElfStringTables(dynElf) == -1) {
    // TODO (mmarchini) error message
    // FIXME (mmarchini) properly free everything
    free(dynElf);
    return NULL;
  }

  dynElf->elf = createElf(fd);

  if ((dynElf->ehdr = createElfHeader(dynElf->elf)) == NULL) {
    // TODO (mmarchini) error message
    // FIXME (mmarchini) properly free everything
    free(dynElf);
    return NULL;
  }

  if(createElfProgramHeaders(dynElf) == -1) {
    // TODO (mmarchini) error message
    // FIXME (mmarchini) properly free everything
    free(dynElf);
    return NULL;
  }

  if(createElfSections(dynElf) == -1) {
    // TODO (mmarchini) error message
    // FIXME (mmarchini) properly free everything
    free(dynElf);
    return NULL;
  }

  return dynElf;
}
