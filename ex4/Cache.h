/*
 * Cache.h
 * represent the whole cache data base
 *  Created on: May 31, 2015
 *      Author: omerom88
 */

#ifndef CACHE_H_
#define CACHE_H_

#include <string>
#include <list>
#include <map>
#include <vector>
#include <fuse.h>
#include <algorithm>
using namespace std;
#include "Block.h"
typedef std::vector<Block*> BlocksVec;
bool compare(const Block* first, const Block* second);

class Cache {
public:
	/*
	 * default constructor
	 */
	Cache(int blockSize, int maxSize);
	/*
	 * empty destructor
	 */
	~Cache();
	/*
	 * function that sort given blocks vector by the blocks counter field
	 */
	BlocksVec sortVector(BlocksVec vec);
	/*
	 * function which responsible to add new block to our cache
	 */
	Block* addNewBlock(const char* path, int blockNum, int fd, int blockSize);
	/*
	 * function that sort list of blocks by the blocks counter field
	 */
	void sortList();
	/*
	 * function that responsible to delete block from the cache
	 */
	void deleteBlockFromCache(Block* toDelete);
	/*
	 * function that free all the cache memory
	 */
	void freeCache();
	int blockSize; //represent the size of a single block in the cache
	int maxSize; //represent the max amount of blocks on the cache
	list<Block*> cacheList; //represent the blocks of the cache as list
	map<string, BlocksVec> fileBlocks; //a map of the cache blocks which maps by file path as key
};

#endif /* CACHE_H_ */
