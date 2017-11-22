/*
 * Thread.cpp
 *
 *  Created on: Apr 13, 2015
 *      Author: omerom88
 */

#include "Thread.h"

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t Thread::translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
		"rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t Thread::translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

/* Ctor for the class */
Thread::Thread(int id, f func ,Priority prio , Status stat) {
	this->_quantumCnt = 0;
	this->_tid = id;
	this->_tprio = prio;
	this->_stats = stat;
	this->_stack = new char[STACK_SIZE];
	this->_func = func;
	this->sp = (address_t)_stack + STACK_SIZE - sizeof(address_t);
	this->pc = (address_t)func;

	sigsetjmp(this->env, 1);
	(this->env->__jmpbuf)[JB_SP] = translate_address(sp);
	(this->env->__jmpbuf)[JB_PC] = translate_address(pc);
	sigemptyset(&this->env->__saved_mask);

}

/* Dtor for the class */
Thread::~Thread() {}

/* Getter for the quantum Counter */
int Thread:: getQuantumCnt() const
{
	return _quantumCnt;
}

/* Getter for the thread id */
int Thread:: getTid() const
{
	return _tid;
}

/* Setter for the thread id */
void Thread:: setTid(int _tid)
{
	this->_tid = _tid;
}

/* Getter for the thread priority */
Priority Thread:: getTprio() const
{
	return _tprio;
}

/* Setter for the thread priority */
void Thread:: setTprio(Priority _tprio)
{
	this->_tprio = _tprio;
}

/* Getter for the thread status */
Status Thread:: getStats() const
{
	return _stats;
}

/* Setter for the thread status */
void Thread:: setStats(Status _stats)
{
	this->_stats = _stats;
}

/* Function that increment quantum Counter */
void Thread::increment()
{
	_quantumCnt++;
}
