#include "tips.h"

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w); 

/* return random int from 0..x-1 */
int randomint( int x );

/*
	This function allows the lfu information to be displayed

		assoc_index - the cache unit that contains the block to be modified
    	block_index - the index of the block to be modified

	returns a string representation of the lfu information
 */
char* lfu_to_string(int assoc_index, int block_index)
{
	/* Buffer to print lfu information -- increase size as needed. */
	static char buffer[9];
	sprintf(buffer, "%u", cache[assoc_index].block[block_index].accessCount);

	return buffer;
}

/*
	This function allows the lru information to be displayed

		assoc_index - the cache unit that contains the block to be modified
		block_index - the index of the block to be modified

	returns a string representation of the lru information
 */
char* lru_to_string(int assoc_index, int block_index)
{
	/* Buffer to print lru information -- increase size as needed. */
	static char buffer[9];
	sprintf(buffer, "%u", cache[assoc_index].block[block_index].lru.value);

	return buffer;
}

/*
	This function initializes the lfu information

		assoc_index - the cache unit that contains the block to be modified
		block_number - the index of the block to be modified
*/
void init_lfu(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].accessCount = 0;
}

/*
	This function initializes the lru information

		assoc_index - the cache unit that contains the block to be modified
		block_number - the index of the block to be modified
*/
void init_lru(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].lru.value = 0;
}

////////////////////////////////////////////////////////////////////////////
//////// Our solution after this line //////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


int end = 1;

unsigned int getMask ( unsigned int num ) {

	switch ( num ) 
	{
		case 1:
			return 0x1;
		case 2:
			return 0x3;
		case 3:
			return 0x7;
		case 4:
			return 0xf;
		case 5:
			return 0x1f;
		case 8:
			return 0xff;
		case 16:
			return 0xffff;
	}

	return 0;
} 


void getAddrInformation ( address addr, unsigned int* tag, unsigned int* index, unsigned int* offset ) {
   
	unsigned int offsetBits = uint_log2(block_size);
	unsigned int indexBits = uint_log2(set_count);

	unsigned int mask = 0;

	mask = getMask(offsetBits);
	*offset = addr & mask;
	addr = addr >> offsetBits;

	mask = getMask(indexBits);
	*index = addr & mask;
	addr = addr >> indexBits;

	*tag = addr;

	return;
}

TransferUnit getTransferUnit ( ) {

	if ( block_size == 4 ) {
		return WORD_SIZE;
	}
	else if ( block_size == 8 ) {
		return DOUBLEWORD_SIZE;
	}
	else if ( block_size == 16 ) {
		return QUADWORD_SIZE;
	}
	else if ( block_size == 32 ) {
		return OCTWORD_SIZE;
	}

	return WORD_SIZE;
}

int getEmptyBlock ( int index ) {

	for ( int i = 0; i < assoc; i++ ) {
		if ( cache[index].block[i].valid == 0 ) {
			return i;
		}
	}

	return -1;
}


void updateLRU ( int set, int blockIndex, int available ) {

	int replacing = cache[set].block[blockIndex].lru.value;
	cache[set].block[blockIndex].lru.value = 1;

	if ( available == -1 ) {

		for ( int i = 0; i < assoc; i++ ) {
			if ( cache[set].block[i].lru.value < replacing && i != blockIndex ) {
				cache[set].block[i].lru.value++;
			}
		}

	}

	else {

		for ( int i = 0; i < assoc; i++ ) {
			if ( cache[set].block[i].valid == 1 && i != blockIndex ) {
				cache[set].block[i].lru.value++;
			}
		}

	}

	return;
}

int LRUToReplace ( int index ) {

	int maximum = cache[index].block[0].lru.value;
	int toReplace = 0;

	for ( int i = 1; i < assoc; i++ ) {
		if (  cache[index].block[i].lru.value > maximum ) {
			maximum =  cache[index].block[i].lru.value;
			toReplace = i;
		}
	}

	return toReplace;
}

void printAddrInfo ( char* msg , int tag, int index, int offset ) {

	printf("%s\n", msg);
	printf("Tag: %x\tIndex: %d\tOffset: %d\n", tag, index, offset);

	return;
}

/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/
void accessMemory(address addr, word* data, WriteEnable we)
{
	/* Declare variables here */

	unsigned int tag = 0, index = 0, offset = 0;
	int notFound = 1;
	TransferUnit toTransfer; 

	int dataIndex = 0;

	/* handle the case of no cache at all - leave this in */
	if(assoc == 0) {
		accessDRAM(addr, (byte*)data, WORD_SIZE, we);
		return;
	}


	toTransfer = getTransferUnit();
	getAddrInformation(addr, &tag, &index, &offset);

	if ( end ) {

		for ( int i = 0; i < assoc; i++ ) {

			if ( tag == cache[index].block[i].tag && cache[index].block[i].valid == 1 ) {
				notFound = 0;
				
				int blockIndex = getEmptyBlock(index);

				if ( cache[index].block[i].lru.value != 1 )
					updateLRU(index, i, blockIndex);

				// Highlight block in green if it matches
				highlight_block(index, i);     
				highlight_offset(index, i, offset, HIT);

				dataIndex = i;

				printAddrInfo("From hit", tag, index, offset);
	
				break;
			} 

		}


		if ( notFound == 1 ) {

			int insertIndex = getEmptyBlock(index);
			int availability = insertIndex;

			if ( availability == -1 ) {
				insertIndex = LRUToReplace(index);
			}

			// DEBUG
			printf("Accessing address: %x\n", addr - offset);
			printf("\tAddress: %x\n", addr);
			printf("\tOffset * 4: %x\n", offset * 4);


			accessDRAM(addr - offset, cache[index].block[insertIndex].data, toTransfer, READ);
			cache[index].block[insertIndex].valid = 1;
			cache[index].block[insertIndex].tag = tag;

			if ( cache[index].block[insertIndex].lru.value != 1 )
				updateLRU(index, insertIndex, availability);

			highlight_block(index, insertIndex);
			highlight_offset(index, insertIndex, offset, MISS);


			// int blockIndex = getEmptyBlock(index);
			// updateLRU(index, 0, blockIndex);

			// accessDRAM(addr, cache[index].block[0].data, toTransfer, READ);
			// cache[index].block[0].tag = tag;
			// cache[index].block[0].valid = 1;
			// highlight_block(index, 0);
			// highlight_offset(index, 0, offset, MISS);

			printAddrInfo("From miss", tag, index, offset);

			dataIndex = insertIndex;
		}

	}

	memcpy((void*)data, (void*)(cache[index].block[dataIndex].data + offset), 4);

	// Stop the program from going
	if ( (long)(*data) ==  0xffffffff ) {
		end = 0;
	} 

	return;
}
