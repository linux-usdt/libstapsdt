#include <err.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <libelf.h>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/types.h>

#include "util.h"
#include "sdtnote.h"
#include "string-table.h"
#include "dynamic-symbols.h"
#include "section.h"

#define PHDR_ALIGN 0x200000
#define PROBE_SYMBOL "lorem"

void _funcStart();
void _funcEnd();

// ------------------------------------------------------------------------- //
// TODO dynamic hashes creation
uint32_t hash_words [] = {
  0x00000003,
  0x00000006,
  0x00000004,
  0x00000005,
  0x00000003,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000002,
  0x00000000,
};

uint32_t eh_frame[] = {0x0, 0x0};

// ------------------------------------------------------------------------- //
// TODO dynamic strings creation

// Static strings
// StringTableNode *shstrtabStr, *hashStr, *dynsymStr, *dynstrStr, *textStr, *ehStr, *dynamicStr, *stapsdtStr, *noteStapsdtStr;

// ------------------------------------------------------------------------- //
// TODO dynamic strings creation
void *createDynSymData(DynamicSymbolTable *table) {
  size_t symbolsCount = (5 + table->count);
  int i;
  DynamicSymbolList *current;
  Elf64_Sym *dynsyms = malloc(sizeof(Elf64_Sym) * (5 + table->count));

  for(i=0; i < symbolsCount; i++) {
    dynsyms[i].st_name  = 0;
    dynsyms[i].st_info  = 0;
    dynsyms[i].st_other = 0;
    dynsyms[i].st_shndx = 0;
    dynsyms[i].st_value = 0;
    dynsyms[i].st_size  = 0;
  }

  dynsyms[1].st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);

  current = table->symbols;
  for(i=0; i < table->count; i++) {
    dynsyms[i + 2].st_name  = current->symbol.string->index;
    dynsyms[i + 2].st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
  }
  i -= 1;


  dynsyms[i + 3].st_name  = table->bssStart.string->index;
  dynsyms[i + 3].st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);

  dynsyms[i + 4].st_name  = table->eData.string->index;
  dynsyms[i + 4].st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);

  dynsyms[i + 5].st_name  = table->end.string->index;
  dynsyms[i + 5].st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);

  return dynsyms;
}

// TODO dynamic strings creation
void *createDynamicData() {
  Elf64_Dyn *dyns = malloc(sizeof(Elf64_Dyn) * 11);

  for(int i=0; i < 11; i++) {
    dyns[i].d_tag = DT_NULL;
  }

  dyns[0].d_tag = DT_HASH;
  dyns[1].d_tag = DT_STRTAB;
  dyns[2].d_tag = DT_SYMTAB;
  dyns[3].d_tag = DT_STRSZ;

  return dyns;
}

Elf *create_elf(int fd) {
  Elf *e;

  if (elf_version(EV_CURRENT) == EV_NONE)
    errx(EXIT_FAILURE, "ELF library initialization failed: %s", elf_errmsg(-1));

  if ((e = elf_begin(fd, ELF_C_WRITE, NULL)) == NULL)
    errx(EXIT_FAILURE, "elf_begin failed: %s", elf_errmsg(-1));

  return e;
}

int createSharedLibrary(int fd, char* provider, char *probe) {
  Elf *e;
  Section hashSection,
          dynSymSection,
          dynStrSection,
          textSection,
          sdtBaseSection,
          ehFrameSection,
          dynamicSection,
          sdtNoteSection,
          shStrTabSection;

  Elf64_Ehdr *ehdr;
  Elf64_Phdr *phdrLoad1,
             *phdrLoad2,
             *phdrDyn,
             *phdrSdtNote;

  // Static strings
  StringTable *shStringTable = stringTableInit();
  shStrTabSection.string = stringTableAdd(shStringTable, ".shstrtab");
  hashSection.string = stringTableAdd(shStringTable, ".hash");
  dynSymSection.string = stringTableAdd(shStringTable, ".dynsym");
  dynStrSection.string = stringTableAdd(shStringTable, ".dynstr");
  textSection.string = stringTableAdd(shStringTable, ".text");
  ehFrameSection.string = stringTableAdd(shStringTable, ".eh_frame");
  dynamicSection.string = stringTableAdd(shStringTable, ".dynamic");
  sdtBaseSection.string = stringTableAdd(shStringTable, ".stapsdt.base");
  sdtNoteSection.string = stringTableAdd(shStringTable, ".note.stapsdt");

  // Dynamic strings
  StringTable *dynamicString = stringTableInit();
  DynamicSymbolTable *dynamicSymbols = dynamicSymbolTableInit(dynamicString);
  dynamicSymbolTableAdd(dynamicSymbols, PROBE_SYMBOL);

  Elf64_Sym *dynSymData = createDynSymData(dynamicSymbols);
  Elf64_Dyn *dynamicData = createDynamicData();

  SDTNote *sdtNote = sdtNoteInit(provider, probe);
  void *sdtNoteData = malloc(sdtNoteSize(sdtNote));


  e = create_elf(fd);

  if((ehdr = elf64_newehdr(e)) == NULL)
    errx(EXIT_FAILURE, "elf64_newehdr failed: %s", elf_errmsg(-1));

  ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
  ehdr->e_type           = ET_DYN;
  ehdr->e_machine        = EM_X86_64;
  ehdr->e_version        = EV_CURRENT;
  ehdr->e_flags          = 0;

  // ----------------------------------------------------------------------- //

  // Create PHDRs

  if((phdrLoad1 = elf64_newphdr(e, 4)) == NULL)
    errx(EXIT_FAILURE, "elf64_newphdr failed: %s", elf_errmsg(-1));

  phdrSdtNote = &phdrLoad1[3];
  phdrDyn = &phdrLoad1[2];
  phdrLoad2 = &phdrLoad1[1];

  // ----------------------------------------------------------------------- //
  // Section: HASH

  if((hashSection.scn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((hashSection.data = elf_newdata(hashSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  hashSection.data->d_align = 8;
  hashSection.data->d_off = 0LL;
  hashSection.data->d_buf = hash_words;
  hashSection.data->d_type = ELF_T_XWORD;
  hashSection.data->d_size = sizeof(hash_words);
  hashSection.data->d_version = EV_CURRENT;

  if((hashSection.shdr = elf64_getshdr(hashSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  hashSection.shdr->sh_name = hashSection.string->index;
  hashSection.shdr->sh_type = SHT_HASH;
  hashSection.shdr->sh_flags = SHF_ALLOC;

  // ----------------------------------------------------------------------- //
  // Section: Dynsym

  if((dynSymSection.scn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((dynSymSection.data = elf_newdata(dynSymSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  dynSymSection.data->d_align = 8;
  dynSymSection.data->d_off = 0LL;
  dynSymSection.data->d_buf = dynSymData;
  dynSymSection.data->d_type = ELF_T_XWORD;
  dynSymSection.data->d_size = sizeof(Elf64_Sym) * 6;
  dynSymSection.data->d_version = EV_CURRENT;

  if((dynSymSection.shdr = elf64_getshdr(dynSymSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  dynSymSection.shdr->sh_name = dynSymSection.string->index;
  dynSymSection.shdr->sh_type = SHT_DYNSYM;
  dynSymSection.shdr->sh_flags = SHF_ALLOC;
  dynSymSection.shdr->sh_info = 2; // First non local symbol

  hashSection.shdr->sh_link = elf_ndxscn(dynSymSection.scn);

  // ----------------------------------------------------------------------- //
  // Section: DYNSTR

  if((dynStrSection.scn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((dynStrSection.data = elf_newdata(dynStrSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  dynStrSection.data->d_align = 1;
  dynStrSection.data->d_off = 0LL;
  dynStrSection.data->d_buf = stringTableToBuffer(dynamicString);

  dynStrSection.data->d_type = ELF_T_BYTE;
  dynStrSection.data->d_size = dynamicString->size;
  dynStrSection.data->d_version = EV_CURRENT;

  if((dynStrSection.shdr = elf64_getshdr(dynStrSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  dynStrSection.shdr->sh_name = dynStrSection.string->index;
  dynStrSection.shdr->sh_type = SHT_STRTAB;
  dynStrSection.shdr->sh_flags = SHF_ALLOC;

  dynSymSection.shdr->sh_link = elf_ndxscn(dynStrSection.scn);


  // ----------------------------------------------------------------------- //
  // Section: TEXT

  if((textSection.scn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((textSection.data = elf_newdata(textSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  textSection.data->d_align = 16;
  textSection.data->d_off = 0LL;
  textSection.data->d_buf = (void *)_funcStart;
  textSection.data->d_type = ELF_T_BYTE;
  textSection.data->d_size = (unsigned long)_funcEnd - (unsigned long)_funcStart;
  textSection.data->d_version = EV_CURRENT;


  if((textSection.shdr = elf64_getshdr(textSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  textSection.shdr->sh_name = textSection.string->index;
  textSection.shdr->sh_type = SHT_PROGBITS;
  textSection.shdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;

  // ----------------------------------------------------------------------- //
  // Section: SDT BASE

  if((sdtBaseSection.scn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((sdtBaseSection.data = elf_newdata(sdtBaseSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  sdtBaseSection.data->d_align = 1;
  sdtBaseSection.data->d_off = 0LL;
  sdtBaseSection.data->d_buf = eh_frame;
  sdtBaseSection.data->d_type = ELF_T_BYTE;
  sdtBaseSection.data->d_size = 1;
  sdtBaseSection.data->d_version = EV_CURRENT;


  if((sdtBaseSection.shdr = elf64_getshdr(sdtBaseSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  sdtBaseSection.shdr->sh_name = sdtBaseSection.string->index;
  sdtBaseSection.shdr->sh_type = SHT_PROGBITS;
  sdtBaseSection.shdr->sh_flags = SHF_ALLOC;

  // ----------------------------------------------------------------------- //
  // Section: EH_FRAME

  if((ehFrameSection.scn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((ehFrameSection.data = elf_newdata(ehFrameSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  ehFrameSection.data->d_align = 8;
  ehFrameSection.data->d_off = 0LL;
  ehFrameSection.data->d_buf = eh_frame;
  ehFrameSection.data->d_type = ELF_T_BYTE;
  ehFrameSection.data->d_size = 0;
  ehFrameSection.data->d_version = EV_CURRENT;


  if((ehFrameSection.shdr = elf64_getshdr(ehFrameSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  ehFrameSection.shdr->sh_name = ehFrameSection.string->index;
  ehFrameSection.shdr->sh_type = SHT_PROGBITS;
  ehFrameSection.shdr->sh_flags = SHF_ALLOC;

  // ----------------------------------------------------------------------- //
  // Section: DYNAMIC

  if((dynamicSection.scn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((dynamicSection.data = elf_newdata(dynamicSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  dynamicSection.data->d_align = 8;
  dynamicSection.data->d_off = 0LL;
  dynamicSection.data->d_buf = dynamicData;
  dynamicSection.data->d_type = ELF_T_BYTE;
  dynamicSection.data->d_size = 11 * sizeof(Elf64_Dyn);
  dynamicSection.data->d_version = EV_CURRENT;


  if((dynamicSection.shdr = elf64_getshdr(dynamicSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  dynamicSection.shdr->sh_name = dynamicSection.string->index;
  dynamicSection.shdr->sh_type = SHT_DYNAMIC;
  dynamicSection.shdr->sh_flags = SHF_WRITE | SHF_ALLOC;
  dynamicSection.shdr->sh_link = elf_ndxscn(dynStrSection.scn);

  // ----------------------------------------------------------------------- //
  // Section: SDT_NOTE

  if((sdtNoteSection.scn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((sdtNoteSection.data = elf_newdata(sdtNoteSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  sdtNoteSection.data->d_align = 4;
  sdtNoteSection.data->d_off = 0LL;
  sdtNoteSection.data->d_buf = sdtNoteData;
  sdtNoteSection.data->d_type = ELF_T_NHDR;
  sdtNoteSection.data->d_size = sdtNoteSize(sdtNote);
  sdtNoteSection.data->d_version = EV_CURRENT;


  if((sdtNoteSection.shdr = elf64_getshdr(sdtNoteSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  sdtNoteSection.shdr->sh_name = sdtNoteSection.string->index;
  sdtNoteSection.shdr->sh_type = SHT_NOTE;
  sdtNoteSection.shdr->sh_flags = 0;

  // ----------------------------------------------------------------------- //
  // Section: SHSTRTAB

  if((shStrTabSection.scn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((shStrTabSection.data = elf_newdata(shStrTabSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  shStrTabSection.data->d_align = 1;
  shStrTabSection.data->d_off = 0LL;
  shStrTabSection.data->d_buf = stringTableToBuffer(shStringTable);
  shStrTabSection.data->d_type = ELF_T_BYTE;
  shStrTabSection.data->d_size = shStringTable->size;
  shStrTabSection.data->d_version = EV_CURRENT;

  if((shStrTabSection.shdr = elf64_getshdr(shStrTabSection.scn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shStrTabSection.shdr->sh_name = shStrTabSection.string->index;
  shStrTabSection.shdr->sh_type = SHT_STRTAB;
  shStrTabSection.shdr->sh_flags = 0;

  ehdr->e_shstrndx = elf_ndxscn(shStrTabSection.scn);

  // ----------------------------------------------------------------------- //

  if (elf_update(e, ELF_C_NULL) < 0)
    errx(EXIT_FAILURE, "elf_update(NULL) failed: %s", elf_errmsg(-1));

  // ----------------------------------------------------------------------- //

  hashSection.shdr->sh_addr = hashSection.shdr->sh_offset;
  hashSection.offset = hashSection.shdr->sh_offset;

  // -- //

  dynSymSection.shdr->sh_addr = dynSymSection.shdr->sh_offset;
  dynSymSection.offset = dynSymSection.shdr->sh_offset;

  // -- //

  dynStrSection.shdr->sh_addr = dynStrSection.shdr->sh_offset;
  dynStrSection.offset = dynStrSection.shdr->sh_offset;

  // -- //

  textSection.shdr->sh_addr = textSection.shdr->sh_offset;
  ehdr->e_entry = textSection.shdr->sh_addr;
  textSection.offset = textSection.shdr->sh_offset;


  // -- //

  sdtBaseSection.shdr->sh_addr = sdtBaseSection.shdr->sh_offset;
  sdtBaseSection.offset = sdtBaseSection.shdr->sh_offset;


  // -- //

  ehFrameSection.shdr->sh_addr = ehFrameSection.shdr->sh_offset;
  ehFrameSection.offset = ehFrameSection.shdr->sh_offset;

  // -- //

  dynamicSection.shdr->sh_addr = PHDR_ALIGN + dynamicSection.shdr->sh_offset;
  dynamicSection.offset = dynamicSection.shdr->sh_offset;

  // -- //

  sdtNoteSection.shdr->sh_addr = sdtNoteSection.shdr->sh_offset;
  sdtNoteSection.offset = sdtNoteSection.shdr->sh_offset;
  sdtNote->content.probePC = textSection.offset;
  sdtNote->content.base_addr = sdtBaseSection.offset;
  sdtNoteToBuffer(sdtNote, sdtNoteData);

  // -- //

  shStrTabSection.offset = shStrTabSection.shdr->sh_offset;

  // -- //

  // ----------------------------------------------------------------------- //

  if (elf_update(e, ELF_C_NULL) < 0)
    errx(EXIT_FAILURE, "elf_update(NULL) failed: %s", elf_errmsg(-1));

  // ----------------------------------------------------------------------- //
  // Fill PHDRs

  // First LOAD PHDR

  phdrLoad1->p_type    = PT_LOAD;
  phdrLoad1->p_flags   = PF_X + PF_R;
  phdrLoad1->p_offset  = 0;
  phdrLoad1->p_vaddr   = 0;
  phdrLoad1->p_paddr   = 0;
  phdrLoad1->p_filesz  = ehFrameSection.offset;
  phdrLoad1->p_memsz   = ehFrameSection.offset;
  phdrLoad1->p_align   = PHDR_ALIGN;

  // Second LOAD PHDR

  phdrLoad2->p_type    = PT_LOAD;
  phdrLoad2->p_flags   = PF_W + PF_R;
  phdrLoad2->p_offset  = ehFrameSection.offset;
  phdrLoad2->p_vaddr   = ehFrameSection.offset + PHDR_ALIGN;
  phdrLoad2->p_paddr   = ehFrameSection.offset + PHDR_ALIGN;
  phdrLoad2->p_filesz  = dynamicSection.data->d_size;
  phdrLoad2->p_memsz   = dynamicSection.data->d_size;
  phdrLoad2->p_align   = PHDR_ALIGN;

  // Dynamic PHDR

  phdrDyn->p_type    = PT_DYNAMIC;
  phdrDyn->p_flags   = PF_W + PF_R;
  phdrDyn->p_offset  = ehFrameSection.offset;
  phdrDyn->p_vaddr   = ehFrameSection.offset + PHDR_ALIGN;
  phdrDyn->p_paddr   = ehFrameSection.offset + PHDR_ALIGN;
  phdrDyn->p_filesz  = dynamicSection.data->d_size;
  phdrDyn->p_memsz   = dynamicSection.data->d_size;
  phdrDyn->p_align   = 0x8;  // XXX magic number?

  // Fix offsets DynSym
  // ----------------------------------------------------------------------- //

  dynSymData[0].st_value = 0;

  dynSymData[1].st_value = textSection.offset;
  dynSymData[1].st_shndx = elf_ndxscn(textSection.scn);

  dynSymData[2].st_value = textSection.offset;
  dynSymData[2].st_shndx = elf_ndxscn(textSection.scn);

  dynSymData[3].st_value = PHDR_ALIGN + shStrTabSection.offset;
  dynSymData[3].st_shndx = elf_ndxscn(dynamicSection.scn);

  dynSymData[4].st_value = PHDR_ALIGN + shStrTabSection.offset;
  dynSymData[4].st_shndx = elf_ndxscn(dynamicSection.scn);

  dynSymData[5].st_value = PHDR_ALIGN + shStrTabSection.offset;
  dynSymData[5].st_shndx = elf_ndxscn(dynamicSection.scn);

  // Fix offsets Dynamic
  // ----------------------------------------------------------------------- //

  dynamicData[0].d_un.d_ptr = hashSection.offset;
  dynamicData[1].d_un.d_ptr = dynStrSection.offset;
  dynamicData[2].d_un.d_ptr = dynSymSection.offset;
  dynamicData[3].d_un.d_val = dynamicString->size;
  dynamicData[4].d_un.d_val = sizeof(Elf64_Sym);

  // ----------------------------------------------------------------------- //

  elf_flagphdr(e, ELF_C_SET, ELF_F_DIRTY);

  if (elf_update(e, ELF_C_WRITE) < 0)
    errx(EXIT_FAILURE, "elf_updateWRITENULL) failed: %s", elf_errmsg(-1));

  (void) elf_end(e);

	/* Finished */
	return 0;
}

void *registerProbe(char *provider, char *probe) {
  int fd;
  void *handle;
  void *fireProbe;
  char filename[sizeof("/tmp/") + sizeof(provider) + sizeof(probe) + sizeof("XXXXXX") + 2];
  char *error;

  sprintf(filename, "/tmp/%s-%s-XXXXXX", provider, probe);

  if ((fd = mkstemp(filename)) < 0)
    return NULL;

  createSharedLibrary(fd, provider, probe);
  handle = dlopen(filename, RTLD_LAZY);
  if (!handle) {
      fputs (dlerror(), stderr);
      return NULL;
  }

  fireProbe = dlsym(handle, PROBE_SYMBOL);

  if ((error = dlerror()) != NULL)  {
      fputs(error, stderr);
      return NULL;
  }
  printf("Eta\n");

  (void) close(fd);
  return fireProbe;
}
