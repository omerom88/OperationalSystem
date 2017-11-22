/*
 * ReadyQueue.h
 *
 *  Created on: Apr 13, 2015
 *      Author: omerom88
 */
#ifndef READYQUEUE_H_
#define READYQUEUE_H_

#include <map>
#include <list>

#include "Thread.h"
using namespace std;

class ReadyQueue {

public:
	/* Ctor for the class */
	ReadyQueue();
	/* Dtor for the class */
	virtual ~ReadyQueue();
	/* Getter for green queue */
	const list<Thread*>& getGreenQueue() const;
	/* Setter for green queue */
	void setGreenQueue(const list<Thread*>& _greenQueue);
	/* Getter for orange queue */
	const list<Thread*>& getOrangeQueue() const;
	/* Setter for orange queue */
	void setOrangeQueue(const list<Thread*>& _orangeQueue);
	/* Getter for red queue */
	const list<Thread*>& getRedQueue() const;
	/* Setter for red queue */
	void setRedQueue(const list<Thread*>& _redQueue);
	/* Function to push a new thread to the right queue */
	void pushThread(Thread* trd);
	/* Function to pop a thread - by priority */
	Thread* popReadyThread();
	/* Function to delete thread */
	void deleteFromQueue(Thread* trd);
	/* Function to get how many threads are ready */
	int QueuesSize();

private:
	list <Thread*> _redQueue;
	list <Thread*> _orangeQueue;
	list <Thread*> _greenQueue;
};

#endif /* READYQUEUE_H_ */
