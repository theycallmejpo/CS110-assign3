#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "inode.h"
#include "diskimg.h"
#include "../cachemem.h"

#define FILE_BLOCKS_IN_IMGSCTR 256


/*
 * Fetch the specified inode from the filesystem. 
 * Return 0 on success, -1 on error.  
 */
int
inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp)
{
	int indexNum = inumber - 1;
	int numInodesInSector = DISKIMG_SECTOR_SIZE / sizeof(struct inode);
	int sectorNumber = 2 + (indexNum / numInodesInSector);
	char buffer[512];

	// cache it up?

	int bytesRead;
	int cacheIndex = isBlockInCache(sectorNumber);
	
	if(cacheIndex > -1) {
		bytesRead = getBlockFromCache(&buffer, cacheIndex);
	} else {
		
		bytesRead = diskimg_readsector(fs->dfd, sectorNumber, &buffer);
		putBlockInCache(sectorNumber, &buffer, bytesRead);	
	}
	
	
	//int bytesRead = diskimg_readsector(fs->dfd, sectorNumber, &buffer);
	
	if(bytesRead == -1) return -1;
	
	memcpy(inp, &buffer[0] + sizeof(struct inode) * (indexNum % numInodesInSector), sizeof(struct inode));
	return 0;
}

/*
 * Get the location of the specified file block of the specified inode.
 * Return the disk block number on success, -1 on error.  
 */
int
inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum)
{
	//printf("Block num = %d\n", blockNum);
	bool isLarge = inp->i_mode & ILARG;
	if(!isLarge) {
		return inp->i_addr[blockNum];
	} else {
		int indirectBlock = blockNum / FILE_BLOCKS_IN_IMGSCTR;
		int blockToRead = indirectBlock;
		uint16_t blocks[FILE_BLOCKS_IN_IMGSCTR]; 
		if(indirectBlock > 6) blockToRead = 7; // Read last element of blocks if the indirect block is bigger than 6	
		
		int numBytesRead;
		int diskBlockNum = inp->i_addr[blockToRead];
		
		// cache it up
		int cacheIndex = isBlockInCache(diskBlockNum);

		if(cacheIndex > -1) {
			numBytesRead = getBlockFromCache(&blocks, cacheIndex);
		} else {
			numBytesRead = diskimg_readsector(fs->dfd, diskBlockNum, &blocks);
			putBlockInCache(diskBlockNum, &blocks, numBytesRead);
		}

	//	numBytesRead = diskimg_readsector(fs->dfd, diskBlockNum, &blocks);
		
		if(indirectBlock <= 6) {	
		
			return blocks[blockNum % FILE_BLOCKS_IN_IMGSCTR];
		
		} else {
			uint16_t buffer[FILE_BLOCKS_IN_IMGSCTR];
			indirectBlock = blockNum - 7 * FILE_BLOCKS_IN_IMGSCTR;
			indirectBlock = indirectBlock / FILE_BLOCKS_IN_IMGSCTR; // calculate the block for the second level of indirection
			
			diskBlockNum = blocks[indirectBlock];
			cacheIndex = isBlockInCache(diskBlockNum);

			if(cacheIndex > -1) {
				numBytesRead = getBlockFromCache(&buffer, cacheIndex);
			} else {
					numBytesRead = diskimg_readsector(fs->dfd, diskBlockNum, &buffer);
					putBlockInCache(diskBlockNum, &buffer, numBytesRead);
			}
				
			if (numBytesRead == -1) return -1;
			return buffer[blockNum % FILE_BLOCKS_IN_IMGSCTR];
		}
	}
	return -1;		
}

/* 
 * Compute the size of an inode from its size0 and size1 fields.
 */
int
inode_getsize(struct inode *inp) 
{
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
