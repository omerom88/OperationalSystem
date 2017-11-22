/*
 * ReadyQueue.cpp
 *
 *  Created on: Apr 13, 2015
 *      Author: omerom88
 */

#include "ReadyQueue.h"
/* Ctor for the class */
ReadyQueue::ReadyQueue() {}

/* Dtor for the class */
ReadyQueue::~ReadyQueue() {}

/* Getter for green queue */
const list<Thread*>& ReadyQueue::getGreenQueue() const {
	return _greenQueue;
}

/* Setter for green queue */
void ReadyQueue::setGreenQueue(const list<Thread*>& _greenQueue) {
	this->_greenQueue = _greenQueue;
}

/* Getter for orange queue */
const list<Thread*>& ReadyQueue::getOrangeQueue() const {
	return _orangeQueue;
}

/* Setter for orange queue */
void ReadyQueue::setOrangeQueue(const list<Thread*>& _orangeQueue)
{
	this->_orangeQueue = _orangeQueue;
}

/* Getter for red queue */
const list<Thread*>& ReadyQueue::getRedQueue() const
{
	return _redQueue;
}

/* Setter for red queue */
void ReadyQueue::setRedQueue(const list<Thread*>& _redQueue)
{
	this->_redQueue = _redQueue;
}

/* Function to push a new thread to the right queue */
void ReadyQueue::pushThread(Thread* trd)
{
	switch(trd->getTprio())
	{
	case RED:
		_redQueue.push_back(trd);
		trd->setStats(READY);
		break;
	case ORANGE:
		_orangeQueue.push_back(trd);
		trd->setStats(READY);
		break;
	case GREEN:
		_greenQueue.push_back(trd);
		trd->setStats(READY);
		break;
	}
}

/* Function to pop a thread - by priority */
Thread* ReadyQueue::popReadyThread()
{
	Thread* trd;
	if(!_redQueue.empty())
	{
		trd = _redQueue.front();
		_redQueue.pop_front();
		return trd;
	}
	else if(!_orangeQueue.empty())
	{
		trd = _orangeQueue.front();
		_orangeQueue.pop_front();
		return trd;
	}
	else
	{
		trd = _greenQueue.front();
		_greenQueue.pop_front();
		return trd;
	}
}

/* Function to delete thread */
void ReadyQueue::deleteFromQueue(Thread* trd)
{
	switch(trd->getTprio())
	{
	case RED:
		_redQueue.remove(trd);
		break;
	case ORANGE:
		_orangeQueue.remove(trd);
		break;
	case GREEN:
		_greenQueue.remove(trd);
		break;
	}
}

/* Function to get how many threads are ready */
int ReadyQueue::QueuesSize()
{
	return (_redQueue.size() + _orangeQueue.size() + _greenQueue.size());
}
