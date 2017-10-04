#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sdtnote.h"
#include "util.h"


// TODO (mmarchini) add other architectures (this only works for x86_64)
char *regMap(int idx) {
    switch (idx) {
      case 0:
      return "rdi";
        break;
      case 1:
        return "rsi";
        break;
      case 2:
        return "rdx";
        break;
      case 3:
        return "rcx";
        break;
      case 4:
        return "r8";
        break;
      case 5:
        return "r9";
        break;
      default:
        return NULL;
    }
}

size_t sdtNoteSize(SDTNote *sdt) {
  size_t size = 0;
  size += sizeof(sdt->header);
  size += sdt->header.n_namesz;
  size += sdt->header.n_descsz;

  size = roundUp(size, 4);

  return size;
}

SDTNote *sdtNoteInit(SDTProbe_t *probe) {
  char buf[100];
  SDTNote *sdt = calloc(sizeof(SDTNote), 1);
  size_t descsz = 0, providersz = strlen(probe->provider->name) + 1,
         probesz = strlen(probe->name) + 1;
  sdt->header.n_type = NT_STAPSDT;
  sdt->header.n_namesz = sizeof(NT_STAPSDT_NAME);

  // TODO(matheus): should add pad if sizeof(NT_STAPSDT)%4 != 0
  sdt->name = calloc(sizeof(NT_STAPSDT_NAME), 1);
  strncpy(sdt->name, NT_STAPSDT_NAME, strlen(NT_STAPSDT_NAME) + 1);

  sdt->content.probePC = -1;
  descsz += sizeof(sdt->content.probePC);
  sdt->content.base_addr = -1;
  descsz += sizeof(sdt->content.base_addr);
  sdt->content.sem_addr = 0;
  descsz += sizeof(sdt->content.sem_addr);

  sdt->content.provider = calloc(providersz, 1);
  descsz += providersz;
  strncpy(sdt->content.provider, probe->provider->name, providersz);

  sdt->content.probe = calloc(probesz, 1);
  descsz += probesz;
  strncpy(sdt->content.probe, probe->name, probesz);

  sdt->content.argFmt = calloc(sizeof(char), 1);
  sdt->content.argFmt[0] = '\0';
  for(int i=0; i < probe->argCount; i++) {
    sprintf(buf, "%d@%%%s", probe->argFmt[i], regMap(i));


    if(i==0) {
      sdt->content.argFmt = realloc(sdt->content.argFmt, strlen(sdt->content.argFmt) + strlen(buf) + 1);
      sprintf(sdt->content.argFmt, "%s", buf);
    } else {
      sdt->content.argFmt = realloc(sdt->content.argFmt, strlen(sdt->content.argFmt) + strlen(buf) + 2);
      sprintf(&(sdt->content.argFmt[strlen(sdt->content.argFmt)]), " %s", buf);
    }
  }
  descsz += strlen(sdt->content.argFmt) + 1;

  sdt->header.n_descsz = descsz;

  return sdt;
}

int sdtNoteToBuffer(SDTNote *sdt, char *buffer) {
  int cur = 0;
  size_t sdtSize = sdtNoteSize(sdt);

  // Header
  memcpy(&(buffer[cur]), &(sdt->header), sizeof(sdt->header));
  cur += sizeof(sdt->header);

  // Name
  memcpy(&(buffer[cur]), sdt->name, sdt->header.n_namesz);
  cur += sdt->header.n_namesz;

  // Content
  memcpy(&(buffer[cur]), &(sdt->content.probePC), sizeof(sdt->content.probePC));
  cur += sizeof(sdt->content.probePC);

  memcpy(&(buffer[cur]), &(sdt->content.base_addr),
         sizeof(sdt->content.base_addr));
  cur += sizeof(sdt->content.base_addr);

  memcpy(&(buffer[cur]), &(sdt->content.sem_addr),
         sizeof(sdt->content.sem_addr));
  cur += sizeof(sdt->content.sem_addr);

  memcpy(&(buffer[cur]), sdt->content.provider,
         strlen(sdt->content.provider) + 1);
  cur += strlen(sdt->content.provider) + 1;

  memcpy(&(buffer[cur]), sdt->content.probe, strlen(sdt->content.probe) + 1);
  cur += strlen(sdt->content.probe) + 1;

  memcpy(&(buffer[cur]), sdt->content.argFmt, strlen(sdt->content.argFmt) + 1);
  cur += strlen(sdt->content.argFmt) + 1;

  if (cur < sdtSize) {
    memset(&(buffer[cur]), 0, sdtSize - cur);
  }

  return sdtSize;
}

void sdtNoteFree(SDTNote *sdtNote) {
  free(sdtNote->name);
  free(sdtNote->content.provider);
  free(sdtNote->content.probe);
  free(sdtNote->content.argFmt);

  free(sdtNote);
}

SDTNoteList_t *sdtNoteListAppend(SDTNoteList_t *list, SDTNote *note) {
  SDTNoteList_t *newNode = calloc(sizeof(SDTNoteList_t), 1);
  newNode->next = list;
  newNode->note = note;

  return newNode;
}

size_t sdtNoteListSize(SDTNoteList_t *list) {
  size_t size = 0;
  for(SDTNoteList_t *node=list; node!=NULL; node=node->next) {
    size += sdtNoteSize(node->note);
  }

  return size;
}

size_t sdtNoteListToBuffer(SDTNoteList_t *list, char *buffer) {
  size_t offset = 0;
  for(SDTNoteList_t *node=list; node!=NULL; node=node->next) {
    offset += sdtNoteToBuffer(node->note, &(buffer[offset]));
  }
  return offset;
}

void sdtNoteListFree(SDTNoteList_t *list) {
  SDTNoteList_t *node, *next;
  for(node=list; node!=NULL; node=next) {
    sdtNoteFree(node->note);
    next = node->next;
    free(node);
  }
}
