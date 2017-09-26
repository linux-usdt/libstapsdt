#include <libelf.h>
#include "dynamic-symbols.h"
#include "string-table.h"

DynamicSymbolTable *dynamicSymbolTableInit(StringTable *dynamicString) {
  DynamicSymbolTable *dynSymTab = (DynamicSymbolTable *)malloc(sizeof(DynamicSymbolTable));

  dynSymTab->stringTable = dynamicString;

  dynSymTab->bssStart.string = stringTableAdd(dynamicString, "__bss_start");
  dynSymTab->eData.string = stringTableAdd(dynamicString, "_edata");
  dynSymTab->end.string = stringTableAdd(dynamicString, "_end");

  dynSymTab->count = 0;
  dynSymTab->symbols = NULL;

  return  dynSymTab;
}

 DynamicSymbol *dynamicSymbolTableAdd(DynamicSymbolTable *table, char *symbolName) {
   DynamicSymbolList *symbol = malloc(sizeof(DynamicSymbol));

   symbol->symbol.string = stringTableAdd(table->stringTable, symbolName);

   symbol->next = table->symbols;
   table->symbols = symbol;
   table->count += 1;

  return  &(symbol->symbol);
}
