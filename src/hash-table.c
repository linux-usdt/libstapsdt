#include "dynamic-symbols.h"

#define possibleBucketSizesCount 17
size_t possibleBucketSizes[] = {1, 3, 7, 13, 31, 61, 127, 251, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521, 131071};

static size_t getRecommendedBucketSize(size_t NumSymbols) {
  // Fallback
  size_t bucketSize = NumSymbols * 2;

  for (int i=0; i < possibleBucketSizesCount; i++) {
    if (NumSymbols < possibleBucketSizes[i]) {
      bucketSize = possibleBucketSizes[i];
      break;
    }
  }

  return bucketSize;
}

// TODO (mmarchini) maybe we should use GNU Hash instead
size_t hashTableFromSymbolTable(DynamicSymbolTable *table, uint32_t **hashTable) {
  DynamicSymbolList *current;
  uint32_t nBuckets = getRecommendedBucketSize(table->count),
           nChains = table->count + 1;
  size_t hashTableSize = (nBuckets + nChains + 2);

  uint32_t *hashTable_ = (uint32_t *)calloc(sizeof(uint32_t), hashTableSize);
  int idx, chainIdx=1;

  hashTable_[0] = nBuckets;
  hashTable_[1] = nChains;

  for(current=table->symbols; current!=NULL; current=current->next) {
    idx = elf_hash(current->symbol.string->str) % nBuckets;
    hashTable_[2 + idx] = chainIdx;
    hashTable_[2 + nBuckets + 1] = chainIdx + 1;
    chainIdx++;
  }

  *hashTable = hashTable_;
  return hashTableSize * sizeof(uint32_t);
}
