/*
 * Block.cpp
 * implement it's hash file
 *  Created on: May 31, 2015
 *      Author: omerom88
 */
#include "Block.h"

Block::Block(string filePath, int fileContentNum, int fd) 
{
	this->filePath = filePath;
	this->fileContentNum = fileContentNum;
	this->fd = fd;
	this->counter = 0;
	this->data = NULL;
}

Block::~Block() 
{}

void Block::deleteBlock() 
{
	delete (this->data);
	delete (this);
}
