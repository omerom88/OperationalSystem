/*
 * Cache.cpp
 * implement it's hash file
 *  Created on: May 31, 2015
 *      Author: omerom88
 */

#include "Cache.h"
#include <iostream>

Cache::Cache(int blockSize, int maxSize) 
{
	this->blockSize = blockSize;
	this->maxSize = maxSize;
}

Cache::~Cache() 
{}

void Cache::sortList() 
{
	cacheList.sort(compare);
}

void Cache::deleteBlockFromCache(Block* toDelete) 
{
	BlocksVec vec;
	std::map<string, BlocksVec>::iterator mapIter;
	//delete from the map vector
	mapIter = fileBlocks.find(toDelete->filePath);
	if (mapIter != fileBlocks.end()) //path not found on map
	{
		vec = mapIter->second;
		vec.erase(std::remove(vec.begin(), vec.end(), toDelete), vec.end());
		if (!vec.empty()) 
		{
			fileBlocks[toDelete->filePath] = vec; //return the new sorted modified vec to the map
		} 
		else //vec is empty now
		{
			fileBlocks.erase(mapIter); //remove the whole vec from the map
		}
	}
	//delete from the cache list
	cacheList.remove(toDelete);
	//delete from the block
	toDelete->deleteBlock();
}

Block* Cache::addNewBlock(const char* path, int blockNum, int fd,
		int blockSize) 
{
	Block* temp = new Block(path, blockNum, fd);
	temp->data = new char[blockSize];
	if ((signed) cacheList.size() == maxSize) 
	{
		Block* toDelete = cacheList.back();
		deleteBlockFromCache(toDelete);
	}
	cacheList.push_back(temp);
	BlocksVec vec;
	std::map<string, BlocksVec>::iterator mapIter;
	mapIter = fileBlocks.find(path);
	if (mapIter == fileBlocks.end()) //path not found on map
	{
		vec = BlocksVec(); //creating new vector to insert to the map
	} 
	else //path found on map means the vector exist
	{
		vec = mapIter->second;
	}
	vec.push_back(temp);
	//sort the vector
	vec = sortVector(vec);
	fileBlocks[path] = vec;
	return temp;
}

bool compare(const Block* first, const Block* second) 
{
	return (first->counter > second->counter);
}

BlocksVec Cache::sortVector(BlocksVec vec) 
{
	std::sort(vec.begin(), vec.end(), compare);
	return vec;
}

void Cache::freeCache() 
{
	list<Block*>::iterator it;
	for (it = cacheList.begin(); it != cacheList.end(); ++it) 
	{
		deleteBlockFromCache((*it));
	}
	fileBlocks.clear();
	cacheList.clear();
}

