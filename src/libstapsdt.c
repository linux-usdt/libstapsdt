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

#define PHDR_ALIGN 0x200000
#define PROBE_SYMBOL "lorem"

void _funcStart();
void _funcEnd();

// ------------------------------------------------------------------------- //
// TODO(mmarchini): dynamic hashes creation
uint32_t hash_words[] = {
    0x00000003, 0x00000006, 0x00000004, 0x00000005, 0x00000003, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000002, 0x00000000,
};

uint32_t eh_frame[] = {0x0, 0x0};

// ------------------------------------------------------------------------- //

// ------------------------------------------------------------------------- //
// TODO(mmarchini): dynamic strings creation
void *createDynSymData(DynamicSymbolTable *table) {
  size_t symbolsCount = (5 + table->count);
  int i;
  DynamicSymbolList *current;
  Elf64_Sym *dynsyms = malloc(sizeof(Elf64_Sym) * (5 + table->count));

  for (i = 0; i < symbolsCount; i++) {
    dynsyms[i].st_name = 0;
    dynsyms[i].st_info = 0;
    dynsyms[i].st_other = 0;
    dynsyms[i].st_shndx = 0;
    dynsyms[i].st_value = 0;
    dynsyms[i].st_size = 0;
  }

  dynsyms[1].st_info = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);

  current = table->symbols;
  for (i = 0; i < table->count; i++) {
    dynsyms[i + 2].st_name = current->symbol.string->index;
    dynsyms[i + 2].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
  }
  i -= 1;

  dynsyms[i + 3].st_name = table->bssStart.string->index;
  dynsyms[i + 3].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);

  dynsyms[i + 4].st_name = table->eData.string->index;
  dynsyms[i + 4].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);

  dynsyms[i + 5].st_name = table->end.string->index;
  dynsyms[i + 5].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);

  return dynsyms;
}

// TODO(mmarchini): dynamic strings creation
void *createDynamicData() {
  Elf64_Dyn *dyns = malloc(sizeof(Elf64_Dyn) * 11);

  for (int i = 0; i < 11; i++) {
    dyns[i].d_tag = DT_NULL;
  }

  dyns[0].d_tag = DT_HASH;
  dyns[1].d_tag = DT_STRTAB;
  dyns[2].d_tag = DT_SYMTAB;
  dyns[3].d_tag = DT_STRSZ;

  return dyns;
}

int createSharedLibrary(int fd, char *provider, char *probe) {
  // Dynamic strings
  DynElf *dynElf = dynElfInit(fd);

  DynamicSymbolTable *dynamicSymbols = dynamicSymbolTableInit(dynElf->dynamicString);
  dynamicSymbolTableAdd(dynamicSymbols, PROBE_SYMBOL);

  Elf64_Sym *dynSymData = createDynSymData(dynamicSymbols);
  Elf64_Dyn *dynamicData = createDynamicData();

  SDTNote *sdtNote = sdtNoteInit(provider, probe);
  void *sdtNoteData = malloc(sdtNoteSize(sdtNote));

  // ----------------------------------------------------------------------- //


  // ----------------------------------------------------------------------- //
  // Section: HASH

  dynElf->sections.hash->data->d_align = 8;
  dynElf->sections.hash->data->d_off = 0LL;
  dynElf->sections.hash->data->d_buf = hash_words;
  dynElf->sections.hash->data->d_type = ELF_T_XWORD;
  dynElf->sections.hash->data->d_size = sizeof(hash_words);
  dynElf->sections.hash->data->d_version = EV_CURRENT;

  dynElf->sections.hash->shdr->sh_name = dynElf->sections.hash->string->index;
  dynElf->sections.hash->shdr->sh_type = SHT_HASH;
  dynElf->sections.hash->shdr->sh_flags = SHF_ALLOC;

  // ----------------------------------------------------------------------- //
  // Section: Dynsym

  dynElf->sections.dynSym->data->d_align = 8;
  dynElf->sections.dynSym->data->d_off = 0LL;
  dynElf->sections.dynSym->data->d_buf = dynSymData;
  dynElf->sections.dynSym->data->d_type = ELF_T_XWORD;
  dynElf->sections.dynSym->data->d_size = sizeof(Elf64_Sym) * 6;
  dynElf->sections.dynSym->data->d_version = EV_CURRENT;

  dynElf->sections.dynSym->shdr->sh_name = dynElf->sections.dynSym->string->index;
  dynElf->sections.dynSym->shdr->sh_type = SHT_DYNSYM;
  dynElf->sections.dynSym->shdr->sh_flags = SHF_ALLOC;
  dynElf->sections.dynSym->shdr->sh_info = 2; // First non local symbol

  dynElf->sections.hash->shdr->sh_link = elf_ndxscn(dynElf->sections.dynSym->scn);

  // ----------------------------------------------------------------------- //
  // Section: DYNSTR

  dynElf->sections.dynStr->data->d_align = 1;
  dynElf->sections.dynStr->data->d_off = 0LL;
  dynElf->sections.dynStr->data->d_buf = stringTableToBuffer(dynElf->dynamicString);

  dynElf->sections.dynStr->data->d_type = ELF_T_BYTE;
  dynElf->sections.dynStr->data->d_size = dynElf->dynamicString->size;
  dynElf->sections.dynStr->data->d_version = EV_CURRENT;

  dynElf->sections.dynStr->shdr->sh_name = dynElf->sections.dynStr->string->index;
  dynElf->sections.dynStr->shdr->sh_type = SHT_STRTAB;
  dynElf->sections.dynStr->shdr->sh_flags = SHF_ALLOC;

  dynElf->sections.dynSym->shdr->sh_link = elf_ndxscn(dynElf->sections.dynStr->scn);

  // ----------------------------------------------------------------------- //
  // Section: TEXT

  dynElf->sections.text->data->d_align = 16;
  dynElf->sections.text->data->d_off = 0LL;
  dynElf->sections.text->data->d_buf = (void *)_funcStart;
  dynElf->sections.text->data->d_type = ELF_T_BYTE;
  dynElf->sections.text->data->d_size =
      (unsigned long)_funcEnd - (unsigned long)_funcStart;
  dynElf->sections.text->data->d_version = EV_CURRENT;

  dynElf->sections.text->shdr->sh_name = dynElf->sections.text->string->index;
  dynElf->sections.text->shdr->sh_type = SHT_PROGBITS;
  dynElf->sections.text->shdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;

  // ----------------------------------------------------------------------- //
  // Section: SDT BASE

  dynElf->sections.sdtBase->data->d_align = 1;
  dynElf->sections.sdtBase->data->d_off = 0LL;
  dynElf->sections.sdtBase->data->d_buf = eh_frame;
  dynElf->sections.sdtBase->data->d_type = ELF_T_BYTE;
  dynElf->sections.sdtBase->data->d_size = 1;
  dynElf->sections.sdtBase->data->d_version = EV_CURRENT;

  dynElf->sections.sdtBase->shdr->sh_name = dynElf->sections.sdtBase->string->index;
  dynElf->sections.sdtBase->shdr->sh_type = SHT_PROGBITS;
  dynElf->sections.sdtBase->shdr->sh_flags = SHF_ALLOC;

  // ----------------------------------------------------------------------- //
  // Section: EH_FRAME

  dynElf->sections.ehFrame->data->d_align = 8;
  dynElf->sections.ehFrame->data->d_off = 0LL;
  dynElf->sections.ehFrame->data->d_buf = eh_frame;
  dynElf->sections.ehFrame->data->d_type = ELF_T_BYTE;
  dynElf->sections.ehFrame->data->d_size = 0;
  dynElf->sections.ehFrame->data->d_version = EV_CURRENT;

  dynElf->sections.ehFrame->shdr->sh_name = dynElf->sections.ehFrame->string->index;
  dynElf->sections.ehFrame->shdr->sh_type = SHT_PROGBITS;
  dynElf->sections.ehFrame->shdr->sh_flags = SHF_ALLOC;

  // ----------------------------------------------------------------------- //
  // Section: DYNAMIC

  dynElf->sections.dynamic->data->d_align = 8;
  dynElf->sections.dynamic->data->d_off = 0LL;
  dynElf->sections.dynamic->data->d_buf = dynamicData;
  dynElf->sections.dynamic->data->d_type = ELF_T_BYTE;
  dynElf->sections.dynamic->data->d_size = 11 * sizeof(Elf64_Dyn);
  dynElf->sections.dynamic->data->d_version = EV_CURRENT;

  dynElf->sections.dynamic->shdr->sh_name = dynElf->sections.dynamic->string->index;
  dynElf->sections.dynamic->shdr->sh_type = SHT_DYNAMIC;
  dynElf->sections.dynamic->shdr->sh_flags = SHF_WRITE | SHF_ALLOC;
  dynElf->sections.dynamic->shdr->sh_link = elf_ndxscn(dynElf->sections.dynStr->scn);

  // ----------------------------------------------------------------------- //
  // Section: SDT_NOTE

  dynElf->sections.sdtNote->data->d_align = 4;
  dynElf->sections.sdtNote->data->d_off = 0LL;
  dynElf->sections.sdtNote->data->d_buf = sdtNoteData;
  dynElf->sections.sdtNote->data->d_type = ELF_T_NHDR;
  dynElf->sections.sdtNote->data->d_size = sdtNoteSize(sdtNote);
  dynElf->sections.sdtNote->data->d_version = EV_CURRENT;

  dynElf->sections.sdtNote->shdr->sh_name = dynElf->sections.sdtNote->string->index;
  dynElf->sections.sdtNote->shdr->sh_type = SHT_NOTE;
  dynElf->sections.sdtNote->shdr->sh_flags = 0;

  // ----------------------------------------------------------------------- //
  // Section: SHSTRTAB

  dynElf->sections.shStrTab->data->d_align = 1;
  dynElf->sections.shStrTab->data->d_off = 0LL;
  dynElf->sections.shStrTab->data->d_buf = stringTableToBuffer(dynElf->stringTable);
  dynElf->sections.shStrTab->data->d_type = ELF_T_BYTE;
  dynElf->sections.shStrTab->data->d_size = dynElf->stringTable->size;
  dynElf->sections.shStrTab->data->d_version = EV_CURRENT;

  dynElf->sections.shStrTab->shdr->sh_name = dynElf->sections.shStrTab->string->index;
  dynElf->sections.shStrTab->shdr->sh_type = SHT_STRTAB;
  dynElf->sections.shStrTab->shdr->sh_flags = 0;

  dynElf->ehdr->e_shstrndx = elf_ndxscn(dynElf->sections.shStrTab->scn);

  // ----------------------------------------------------------------------- //

  if (elf_update(dynElf->elf, ELF_C_NULL) < 0) {
    errx(EXIT_FAILURE, "elf_update(NULL) failed: %s", elf_errmsg(-1));
  }

  // ----------------------------------------------------------------------- //

  dynElf->sections.hash->shdr->sh_addr = dynElf->sections.hash->shdr->sh_offset;
  dynElf->sections.hash->offset = dynElf->sections.hash->shdr->sh_offset;

  // -- //

  dynElf->sections.dynSym->shdr->sh_addr = dynElf->sections.dynSym->shdr->sh_offset;
  dynElf->sections.dynSym->offset = dynElf->sections.dynSym->shdr->sh_offset;

  // -- //

  dynElf->sections.dynStr->shdr->sh_addr = dynElf->sections.dynStr->shdr->sh_offset;
  dynElf->sections.dynStr->offset = dynElf->sections.dynStr->shdr->sh_offset;

  // -- //

  dynElf->sections.text->shdr->sh_addr = dynElf->sections.text->shdr->sh_offset;
  dynElf->ehdr->e_entry = dynElf->sections.text->shdr->sh_addr;
  dynElf->sections.text->offset = dynElf->sections.text->shdr->sh_offset;

  // -- //

  dynElf->sections.sdtBase->shdr->sh_addr = dynElf->sections.sdtBase->shdr->sh_offset;
  dynElf->sections.sdtBase->offset = dynElf->sections.sdtBase->shdr->sh_offset;

  // -- //

  dynElf->sections.ehFrame->shdr->sh_addr = dynElf->sections.ehFrame->shdr->sh_offset;
  dynElf->sections.ehFrame->offset = dynElf->sections.ehFrame->shdr->sh_offset;

  // -- //

  dynElf->sections.dynamic->shdr->sh_addr = PHDR_ALIGN + dynElf->sections.dynamic->shdr->sh_offset;
  dynElf->sections.dynamic->offset = dynElf->sections.dynamic->shdr->sh_offset;

  // -- //

  dynElf->sections.sdtNote->shdr->sh_addr = dynElf->sections.sdtNote->shdr->sh_offset;
  dynElf->sections.sdtNote->offset = dynElf->sections.sdtNote->shdr->sh_offset;
  sdtNote->content.probePC = dynElf->sections.text->offset;
  sdtNote->content.base_addr = dynElf->sections.sdtBase->offset;
  sdtNoteToBuffer(sdtNote, sdtNoteData);

  // -- //

  dynElf->sections.shStrTab->offset = dynElf->sections.shStrTab->shdr->sh_offset;

  // -- //

  // ----------------------------------------------------------------------- //

  if (elf_update(dynElf->elf, ELF_C_NULL) < 0) {
    errx(EXIT_FAILURE, "elf_update(NULL) failed: %s", elf_errmsg(-1));
  }

  // ----------------------------------------------------------------------- //
  // Fill PHDRs

  // First LOAD PHDR

  dynElf->phdrLoad1->p_type = PT_LOAD;
  dynElf->phdrLoad1->p_flags = PF_X + PF_R;
  dynElf->phdrLoad1->p_offset = 0;
  dynElf->phdrLoad1->p_vaddr = 0;
  dynElf->phdrLoad1->p_paddr = 0;
  dynElf->phdrLoad1->p_filesz = dynElf->sections.ehFrame->offset;
  dynElf->phdrLoad1->p_memsz = dynElf->sections.ehFrame->offset;
  dynElf->phdrLoad1->p_align = PHDR_ALIGN;

  // Second LOAD PHDR

  dynElf->phdrLoad2->p_type = PT_LOAD;
  dynElf->phdrLoad2->p_flags = PF_W + PF_R;
  dynElf->phdrLoad2->p_offset = dynElf->sections.ehFrame->offset;
  dynElf->phdrLoad2->p_vaddr = dynElf->sections.ehFrame->offset + PHDR_ALIGN;
  dynElf->phdrLoad2->p_paddr = dynElf->sections.ehFrame->offset + PHDR_ALIGN;
  dynElf->phdrLoad2->p_filesz = dynElf->sections.dynamic->data->d_size;
  dynElf->phdrLoad2->p_memsz = dynElf->sections.dynamic->data->d_size;
  dynElf->phdrLoad2->p_align = PHDR_ALIGN;

  // Dynamic PHDR

  dynElf->phdrDyn->p_type = PT_DYNAMIC;
  dynElf->phdrDyn->p_flags = PF_W + PF_R;
  dynElf->phdrDyn->p_offset = dynElf->sections.ehFrame->offset;
  dynElf->phdrDyn->p_vaddr = dynElf->sections.ehFrame->offset + PHDR_ALIGN;
  dynElf->phdrDyn->p_paddr = dynElf->sections.ehFrame->offset + PHDR_ALIGN;
  dynElf->phdrDyn->p_filesz = dynElf->sections.dynamic->data->d_size;
  dynElf->phdrDyn->p_memsz = dynElf->sections.dynamic->data->d_size;
  dynElf->phdrDyn->p_align = 0x8; // XXX magic number?

  // Fix offsets DynSym
  // ----------------------------------------------------------------------- //

  dynSymData[0].st_value = 0;

  dynSymData[1].st_value = dynElf->sections.text->offset;
  dynSymData[1].st_shndx = elf_ndxscn(dynElf->sections.text->scn);

  dynSymData[2].st_value = dynElf->sections.text->offset;
  dynSymData[2].st_shndx = elf_ndxscn(dynElf->sections.text->scn);

  dynSymData[3].st_value = PHDR_ALIGN + dynElf->sections.shStrTab->offset;
  dynSymData[3].st_shndx = elf_ndxscn(dynElf->sections.dynamic->scn);

  dynSymData[4].st_value = PHDR_ALIGN + dynElf->sections.shStrTab->offset;
  dynSymData[4].st_shndx = elf_ndxscn(dynElf->sections.dynamic->scn);

  dynSymData[5].st_value = PHDR_ALIGN + dynElf->sections.shStrTab->offset;
  dynSymData[5].st_shndx = elf_ndxscn(dynElf->sections.dynamic->scn);

  // Fix offsets Dynamic
  // ----------------------------------------------------------------------- //

  dynamicData[0].d_un.d_ptr = dynElf->sections.hash->offset;
  dynamicData[1].d_un.d_ptr = dynElf->sections.dynStr->offset;
  dynamicData[2].d_un.d_ptr = dynElf->sections.dynSym->offset;
  dynamicData[3].d_un.d_val = dynElf->dynamicString->size;
  dynamicData[4].d_un.d_val = sizeof(Elf64_Sym);

  // ----------------------------------------------------------------------- //

  elf_flagphdr(dynElf->elf, ELF_C_SET, ELF_F_DIRTY);

  if (elf_update(dynElf->elf, ELF_C_WRITE) < 0) {
    errx(EXIT_FAILURE, "elf_updateWRITENULL) failed: %s", elf_errmsg(-1));
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
  printf("Eta\n");

  (void)close(fd);
  return fireProbe;
}
