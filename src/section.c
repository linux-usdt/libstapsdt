#include "section.h"

Section *sectionInit(Elf *e, StringTable *table, char *name) {
  Section *section = malloc(sizeof(Section));

  section->string = stringTableAdd(table, name);

  if ((section->scn = elf_newscn(e)) == NULL) {
    free(section);
    return NULL;
  }

  if ((section->data = elf_newdata(section->scn)) == NULL) {
    free(section);
    return NULL;
  }

  if ((section->shdr = elf64_getshdr(section->scn)) == NULL) {
    free(section);
    return NULL;
  }

  return section;
}
