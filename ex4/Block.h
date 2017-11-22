/*
 * Block.h
 * represent a block of the cache
 *  Created on: May 31, 2015
 *      Author: omerom88
 */

#ifndef BLOCK_H_
#define BLOCK_H_
#include <string>
using namespace std;

class Block {
public:
	/*
	 * default constructor
	 */
	Block(string filePath, int fileContentNum, int fd);
	/*
	 * empty destructor
	 */
	~Block();
	/*
	 * function that responsible to delete block
	 */
	void deleteBlock();
	string filePath; //represent the path of the file which the block's content connect to
	char* data; //represent the content of the block
	int fd; //represent
	int counter; //represent how much times this block has been used
	int fileContentNum; //the relative number of block's content from the file content
};

#endif /* BLOCK_H_ */
