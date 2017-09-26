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

#define PHDR_ALIGN 0x200000

int lorem();
void lorem_end();

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
StringTableNode *shstrtabStr, *hashStr, *dynsymStr, *dynstrStr, *textStr, *ehStr, *dynamicStr, *stapsdtStr, *noteStapsdtStr;

// Dynamic strings
StringTableNode *loremStr, *edataStr, *bssStr, *endStr;

// ------------------------------------------------------------------------- //
// TODO dynamic strings creation
void *createDynSymData() {
  Elf64_Sym *dynsyms = malloc(sizeof(Elf64_Sym) * 6);

  // Empty
  dynsyms[0].st_name  = 0;
  dynsyms[0].st_info  = 0;
  dynsyms[0].st_other = 0;
  dynsyms[0].st_shndx = 0;
  dynsyms[0].st_value = 0;
  dynsyms[0].st_size  = 0;

  dynsyms[1].st_name  = 0;
  dynsyms[1].st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);
  dynsyms[1].st_other = 0;
  dynsyms[1].st_shndx = 0; // elf_ndxscn(textScn);
  dynsyms[1].st_value = 0; // Text section addr
  dynsyms[1].st_size  = 0;

  dynsyms[2].st_name  = loremStr->index;  // FIXME
  dynsyms[2].st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
  dynsyms[2].st_other = 0;
  dynsyms[2].st_shndx = 0; // elf_ndxscn(textScn);
  dynsyms[2].st_value = 0; // Function addr (same as text for now)
  dynsyms[2].st_size  = 0;

  dynsyms[3].st_name  = bssStr->index;  // FIXME
  dynsyms[3].st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
  dynsyms[3].st_other = 0;
  dynsyms[3].st_shndx = 0; // elf_ndxscn(dynScn);
  dynsyms[3].st_value = PHDR_ALIGN + 0x0; // 0x0 should be first address after dynamic section
  dynsyms[3].st_size  = 0;

  dynsyms[4].st_name  = edataStr->index;
  dynsyms[4].st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
  dynsyms[4].st_other = 0;
  dynsyms[4].st_shndx = 0; // elf_ndxscn(dynScn);
  dynsyms[4].st_value = PHDR_ALIGN + 0x0; // 0x0 should be first address after dynamic section
  dynsyms[4].st_size  = 0;

  dynsyms[5].st_name  = endStr->index;
  dynsyms[5].st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
  dynsyms[5].st_other = 0;
  dynsyms[5].st_shndx = 0; // elf_ndxscn(dynScn);
  dynsyms[5].st_value = PHDR_ALIGN + 0x0; // 0x0 should be first address after dynamic section
  dynsyms[5].st_size  = 0;

  return dynsyms;
}

// TODO dynamic strings creation
void *createDynamicData() {
  Elf64_Dyn *dyns = malloc(sizeof(Elf64_Dyn) * 11);

  // HASH

  dyns[0].d_tag = DT_HASH;
  dyns[0].d_un.d_ptr  = 0;

  //

  dyns[1].d_tag = DT_STRTAB;
  dyns[1].d_un.d_ptr  = 0;

  //

  dyns[2].d_tag = DT_SYMTAB;
  dyns[2].d_un.d_ptr = 0;

  //

  dyns[3].d_tag = DT_STRSZ;
  dyns[3].d_un.d_val  = 0;

  //

  dyns[4].d_tag = DT_SYMENT;
  dyns[4].d_un.d_val  = 0;

  //

  dyns[5].d_tag = DT_NULL;
  dyns[5].d_un.d_val  = 0;

  //

  dyns[6].d_tag = DT_NULL;
  dyns[6].d_un.d_val  = 0;

  //

  dyns[7].d_tag = DT_NULL;
  dyns[7].d_un.d_val  = 0;

  //

  dyns[8].d_tag = DT_NULL;
  dyns[8].d_un.d_val  = 0;

  //

  dyns[9].d_tag = DT_NULL;
  dyns[9].d_un.d_val  = 0;

  //

  dyns[10].d_tag = DT_NULL;
  dyns[10].d_un.d_val  = 0;

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
  Elf_Scn *hashScn,
          *dynSymScn,
          *dynStrScn,
          *textScn,
          *sdtBaseScn,
          *ehFrameScn,
          *dynamicScn,
          *sdtNoteScn,
          *shStrTabScn;
  Elf_Data *data;
  Elf64_Ehdr *ehdr;
  Elf64_Phdr *phdrLoad1,
             *phdrLoad2,
             *phdrDyn,
             *phdrSdtNote;
  Elf64_Shdr *shdr;

  // Static strings
  StringTable *shStringTable = stringTableInit();
  shstrtabStr = stringTableAdd(shStringTable, ".shstrtab");
  hashStr = stringTableAdd(shStringTable, ".hash");
  dynsymStr = stringTableAdd(shStringTable, ".dynsym");
  dynstrStr = stringTableAdd(shStringTable, ".dynstr");
  textStr = stringTableAdd(shStringTable, ".text");
  ehStr = stringTableAdd(shStringTable, ".eh_frame");
  dynamicStr = stringTableAdd(shStringTable, ".dynamic");
  stapsdtStr = stringTableAdd(shStringTable, ".stapsdt.base");
  noteStapsdtStr = stringTableAdd(shStringTable, ".note.stapsdt");

  // Dynamic strings
  StringTable *dynamicString = stringTableInit();
  edataStr = stringTableAdd(dynamicString, "_edata");
  bssStr = stringTableAdd(dynamicString, "__bss_start");
  endStr = stringTableAdd(dynamicString, "_end");
  loremStr = stringTableAdd(dynamicString, "lorem");

  Elf64_Sym *dynSymData = createDynSymData();
  Elf64_Dyn *dynamicData = createDynamicData();

  Elf64_Addr
      dynamicSize = 0,
      hashOffset,
      dynSymOffset,
      dynStrOffset,
      textOffset,
      sdtBaseOffset,
      ehFrameOffset,
      dynamicOffset,
      sdtNoteOffset,
      shStrTabOffset;


  // SDTNote *sdtNote = sdtNoteInit("mainer", "lorem");
  SDTNote *sdtNote = sdtNoteInit(provider, probe);
  void *sdtNoteData = malloc(sdtNoteSize(sdtNote));


  e = create_elf(fd);

  if((ehdr = elf64_newehdr(e)) == NULL)
    errx(EXIT_FAILURE, "elf64_newehdr failed: %s", elf_errmsg(-1));

  ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
  ehdr->e_type           = ET_DYN;
  ehdr->e_machine        = EM_X86_64;
  ehdr->e_version        = EV_CURRENT;
  // ehdr->e_entry          = 0x0;       // FIXME: entry should be text
  // ehdr->phoff            = -1;
  // ehdr->shoff            = -1;
  ehdr->e_flags          = 0;
  // ehdr->e_ehsize      = -1;
  // ehdr->e_phentsize      = -1;
  // ehdr->e_phnum      = 3;
  // ehdr->e_shentsize      = -1;
  // ehdr->e_shnum      = -1;
  // ehdr->e_shstrndx      = -1;

  // ----------------------------------------------------------------------- //

  // Create PHDRs

  if((phdrLoad1 = elf64_newphdr(e, 4)) == NULL)
    errx(EXIT_FAILURE, "elf64_newphdr failed: %s", elf_errmsg(-1));

  phdrSdtNote = &phdrLoad1[3];
  phdrDyn = &phdrLoad1[2];
  phdrLoad2 = &phdrLoad1[1];

  // ----------------------------------------------------------------------- //
  // Section: HASH

  if((hashScn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((data = elf_newdata(hashScn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  data->d_align = 8;
  data->d_off = 0LL;
  data->d_buf = hash_words;
  data->d_type = ELF_T_XWORD;
  data->d_size = sizeof(hash_words);
  data->d_version = EV_CURRENT;

  if((shdr = elf64_getshdr(hashScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_name = hashStr->index;
  shdr->sh_type = SHT_HASH;
  shdr->sh_flags = SHF_ALLOC;

  // ----------------------------------------------------------------------- //
  // Section: Dynsym

  if((dynSymScn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((data = elf_newdata(dynSymScn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  data->d_align = 8;
  data->d_off = 0LL;
  data->d_buf = dynSymData;
  data->d_type = ELF_T_XWORD;
  data->d_size = sizeof(Elf64_Sym) * 6;
  data->d_version = EV_CURRENT;

  if((shdr = elf64_getshdr(dynSymScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_name = dynsymStr->index;
  shdr->sh_type = SHT_DYNSYM;
  shdr->sh_flags = SHF_ALLOC;
  shdr->sh_info = 2; // First non local symbol


  if((shdr = elf64_getshdr(hashScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_link = elf_ndxscn(dynSymScn);

  // ----------------------------------------------------------------------- //
  // Section: DYNSTR

  if((dynStrScn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((data = elf_newdata(dynStrScn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  data->d_align = 1;
  data->d_off = 0LL;
  data->d_buf = stringTableToBuffer(dynamicString);

  data->d_type = ELF_T_BYTE;
  data->d_size = dynamicString->size;
  data->d_version = EV_CURRENT;

  if((shdr = elf64_getshdr(dynStrScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_name = dynstrStr->index;
  shdr->sh_type = SHT_STRTAB;
  shdr->sh_flags = SHF_ALLOC;

  if((shdr = elf64_getshdr(dynSymScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_link = elf_ndxscn(dynStrScn);


  // ----------------------------------------------------------------------- //
  // Section: TEXT

  if((textScn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((data = elf_newdata(textScn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  data->d_align = 16;
  data->d_off = 0LL;
  data->d_buf = (void *)lorem;
  data->d_type = ELF_T_BYTE;
  data->d_size = (unsigned long)lorem_end - (unsigned long)lorem;
  data->d_version = EV_CURRENT;


  if((shdr = elf64_getshdr(textScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_name = textStr->index;
  shdr->sh_type = SHT_PROGBITS;
  shdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;

  // ----------------------------------------------------------------------- //
  // Section: SDT BASE

  if((sdtBaseScn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((data = elf_newdata(sdtBaseScn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  data->d_align = 1;
  data->d_off = 0LL;
  data->d_buf = eh_frame;
  data->d_type = ELF_T_BYTE;
  data->d_size = 1;
  data->d_version = EV_CURRENT;


  if((shdr = elf64_getshdr(sdtBaseScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_name = stapsdtStr->index;
  shdr->sh_type = SHT_PROGBITS;
  shdr->sh_flags = SHF_ALLOC;

  // ----------------------------------------------------------------------- //
  // Section: EH_FRAME

  if((ehFrameScn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((data = elf_newdata(ehFrameScn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  data->d_align = 8;
  data->d_off = 0LL;
  data->d_buf = eh_frame;
  data->d_type = ELF_T_BYTE;
  data->d_size = 0;
  data->d_version = EV_CURRENT;


  if((shdr = elf64_getshdr(ehFrameScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_name = ehStr->index;
  shdr->sh_type = SHT_PROGBITS;
  shdr->sh_flags = SHF_ALLOC;

  // ----------------------------------------------------------------------- //
  // Section: DYNAMIC

  if((dynamicScn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((data = elf_newdata(dynamicScn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  data->d_align = 8;
  data->d_off = 0LL;
  data->d_buf = dynamicData;
  data->d_type = ELF_T_BYTE;
  data->d_size = 11 * sizeof(Elf64_Dyn);
  data->d_version = EV_CURRENT;


  if((shdr = elf64_getshdr(dynamicScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_name = dynamicStr->index;
  shdr->sh_type = SHT_DYNAMIC;
  shdr->sh_flags = SHF_WRITE | SHF_ALLOC;
  shdr->sh_link = elf_ndxscn(dynStrScn);

  // ----------------------------------------------------------------------- //
  // Section: SDT_NOTE

  if((sdtNoteScn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((data = elf_newdata(sdtNoteScn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  data->d_align = 4;
  data->d_off = 0LL;
  data->d_buf = sdtNoteData;
  data->d_type = ELF_T_NHDR;
  data->d_size = sdtNoteSize(sdtNote);
  data->d_version = EV_CURRENT;


  if((shdr = elf64_getshdr(sdtNoteScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_name = noteStapsdtStr->index;
  shdr->sh_type = SHT_NOTE;
  shdr->sh_flags = 0;

  // ----------------------------------------------------------------------- //
  // Section: SHSTRTAB

  if((shStrTabScn = elf_newscn(e)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

  if((data = elf_newdata(shStrTabScn)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));

  data->d_align = 1;
  data->d_off = 0LL;
  data->d_buf = stringTableToBuffer(shStringTable);
  data->d_type = ELF_T_BYTE;
  data->d_size = shStringTable->size;
  data->d_version = EV_CURRENT;

  if((shdr = elf64_getshdr(shStrTabScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_name = shstrtabStr->index;
  shdr->sh_type = SHT_STRTAB;
  shdr->sh_flags = 0;

  ehdr->e_shstrndx = elf_ndxscn(shStrTabScn);

  // ----------------------------------------------------------------------- //

  if (elf_update(e, ELF_C_NULL) < 0)
    errx(EXIT_FAILURE, "elf_update(NULL) failed: %s", elf_errmsg(-1));

  // ----------------------------------------------------------------------- //

  if((shdr = elf64_getshdr(hashScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_addr = shdr->sh_offset;
  hashOffset = shdr->sh_offset;

  // -- //

  if((shdr = elf64_getshdr(dynSymScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_addr = shdr->sh_offset;
  dynSymOffset = shdr->sh_offset;

  // -- //

  if((shdr = elf64_getshdr(dynStrScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_addr = shdr->sh_offset;
  dynStrOffset = shdr->sh_offset;

  // -- //

  if((shdr = elf64_getshdr(textScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_addr = shdr->sh_offset;
  ehdr->e_entry = shdr->sh_addr;
  textOffset = shdr->sh_offset;


  // -- //

  if((shdr = elf64_getshdr(sdtBaseScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_addr = shdr->sh_offset;
  sdtBaseOffset = shdr->sh_offset;


  // -- //

  if((shdr = elf64_getshdr(ehFrameScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_addr = shdr->sh_offset;
  ehFrameOffset = shdr->sh_offset;

  // -- //

  if((shdr = elf64_getshdr(dynamicScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_addr = PHDR_ALIGN + shdr->sh_offset;
  dynamicOffset = shdr->sh_offset;

  // -- //

  if((shdr = elf64_getshdr(sdtNoteScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shdr->sh_addr = shdr->sh_offset;
  sdtNoteOffset = shdr->sh_offset;
  sdtNote->content.probePC = textOffset;
  sdtNote->content.base_addr = sdtBaseOffset;
  sdtNoteToBuffer(sdtNote, sdtNoteData);

  // -- //

  if((shdr = elf64_getshdr(shStrTabScn)) == NULL)
    errx(EXIT_FAILURE, "elf64_getshdr failed: %s", elf_errmsg(-1));

  shStrTabOffset = shdr->sh_offset;
  dynamicSize = shStrTabOffset - dynamicOffset;

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
  phdrLoad1->p_filesz  = ehFrameOffset;   // FIXME: should be address at the end of text section
  phdrLoad1->p_memsz   = ehFrameOffset;   // FIXME: should be address at the end of text section
  phdrLoad1->p_align   = PHDR_ALIGN;

  // Second LOAD PHDR

  phdrLoad2->p_type    = PT_LOAD;
  phdrLoad2->p_flags   = PF_W + PF_R;
  phdrLoad2->p_offset  = ehFrameOffset;
  phdrLoad2->p_vaddr   = ehFrameOffset + PHDR_ALIGN;
  phdrLoad2->p_paddr   = ehFrameOffset + PHDR_ALIGN;
  phdrLoad2->p_filesz  = dynamicSize;               // FIXME: should be size of dynamic section
  phdrLoad2->p_memsz   = dynamicSize;               // FIXME: should be size of dynamic section
  phdrLoad2->p_align   = PHDR_ALIGN;

  // Dynamic PHDR

  phdrDyn->p_type    = PT_DYNAMIC;
  phdrDyn->p_flags   = PF_W + PF_R;
  phdrDyn->p_offset  = ehFrameOffset;
  phdrDyn->p_vaddr   = ehFrameOffset + PHDR_ALIGN;
  phdrDyn->p_paddr   = ehFrameOffset + PHDR_ALIGN;
  phdrDyn->p_filesz  = dynamicSize;               // FIXME: should be size of dynamic section
  phdrDyn->p_memsz   = dynamicSize;               // FIXME: should be size of dynamic section
  phdrDyn->p_align   = 0x8;  // XXX magic number?

  // Fix offsets DynSym
  // ----------------------------------------------------------------------- //

  dynSymData[0].st_value = 0;

  dynSymData[1].st_value = textOffset;
  dynSymData[1].st_shndx = elf_ndxscn(textScn);

  dynSymData[2].st_value = textOffset;
  dynSymData[2].st_shndx = elf_ndxscn(textScn);

  dynSymData[3].st_value = PHDR_ALIGN + shStrTabOffset;
  dynSymData[3].st_shndx = elf_ndxscn(dynamicScn);

  dynSymData[4].st_value = PHDR_ALIGN + shStrTabOffset;
  dynSymData[4].st_shndx = elf_ndxscn(dynamicScn);

  dynSymData[5].st_value = PHDR_ALIGN + shStrTabOffset;
  dynSymData[5].st_shndx = elf_ndxscn(dynamicScn);

  // Fix offsets Dynamic
  // ----------------------------------------------------------------------- //

  dynamicData[0].d_un.d_ptr = hashOffset;
  dynamicData[1].d_un.d_ptr = dynStrOffset;
  dynamicData[2].d_un.d_ptr = dynSymOffset;
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

  fireProbe = dlsym(handle, "lorem");

  if ((error = dlerror()) != NULL)  {
      fputs(error, stderr);
      return NULL;
  }

  (void) close(fd);
  return fireProbe;
}
