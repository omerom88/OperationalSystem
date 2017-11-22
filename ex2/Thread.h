/*
 * Thread.h
 *
 *  Created on: Apr 13, 2015
 *      Author: omerom88
 */

#ifndef THREAD_H_
#define THREAD_H_

#include <signal.h>
#include <setjmp.h>
#include "uthreads.h"

typedef unsigned long address_t;

typedef enum Status { READY, RUNNIG, BLOCKED } Status;
typedef void (*f)(void);

class Thread {
public:
	/* Ctor for the class */
	Thread(int id, f func ,Priority prio , Status stat);
	/* Dtor for the class */
	virtual ~Thread();
	/* Getter for the quantum Counter */
	int getQuantumCnt() const;
	/* Getter for the thread id */
	int getTid() const;
	/* Setter for the thread id */
	void setTid(int tid);
	/* Getter for the thread priority */
	Priority getTprio() const;
	/* Setter for the thread priority */
	void setTprio(Priority tprio);
	/* Getter for the thread status */
	Status getStats() const;
	/* Setter for the thread status */
	void setStats(Status stats);
	/* Function that increment quantum Counter */
	void increment();
	address_t translate_address(address_t addr);
	sigjmp_buf env;

private:
	int _tid;
	int _quantumCnt;
	Priority _tprio;
	Status _stats;
	char* _stack;
	address_t sp;
	address_t pc;
	f _func;
};

#endif /* THREAD_H_ */
