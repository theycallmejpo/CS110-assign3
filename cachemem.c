/*
 * cachemem.c  -  This module allocates the memory for caches. 
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h> // for PRIu64

#include <sys/mman.h>

#include "cachemem.h"

int cacheMemSizeInKB;
void *cacheMemPtr;
static int maxNumBlocks;
static int numOfBlocks;

/*
 * Allocate memory of the specified size for the data cache optimizations
 * Return -1 on error, 0 on success. 
 */
int
CacheMem_Init(int sizeInKB)
{
  /* Size needs to be not negative or too big and 
   * multiple of the 4KB page size 
   */
  if ((sizeInKB < 0) || (sizeInKB > (CACHEMEM_MAX_SIZE/1024))
      || (sizeInKB % 4)) {
    fprintf(stderr, "Bad cache size %d\n", sizeInKB);
    return -1;
  }
  void *memPtr = mmap(NULL, sizeInKB*1024, PROT_READ|PROT_WRITE, 
		      MAP_PRIVATE|MAP_ANON, -1, 0);
  if (memPtr == MAP_FAILED) {
    perror("mmap");
    return -1;
  }
  cacheMemSizeInKB = sizeInKB;
  cacheMemPtr = memPtr;
  maxNumBlocks = cacheMemSizeInKB * 1024 / FILE_BLOCK_SIZE; 
  numOfBlocks = 0;
  return 0;
}

/*
 * Puts a block in the cache, and replaces it with another if maximum capacity is exceeded
 */


void putBlockInCache(int diskBlockNumber, void *buf, int bytesRead)
{
  struct cacheBlock *placeForBlock = ((struct cacheBlock *)cacheMemPtr) + numOfBlocks;
  placeForBlock->diskBlockNumber = diskBlockNumber;
  memcpy((char *)placeForBlock + sizeof(int), buf, bytesRead); // copy the content of the parameter buf into the place found for the block in cache
  numOfBlocks++;

}


/*
 * Returns size of the cache
 */

int totalCacheSize()
{
  return numOfBlocks * sizeof(struct cacheBlock);
}

/*
 * Iterates through cache to find element
 */

int isBlockInCache(int diskBlockNumber)
{
  
  for(int i = 0; i < numOfBlocks; i++) {
  	if(((struct cacheBlock *)cacheMemPtr)[i].diskBlockNumber == diskBlockNumber)	// iterate through blocks to find if a cacheBlock exists associated with the parameter
		return 1;
  }

  return -1;

}


