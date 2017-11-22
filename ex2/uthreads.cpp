/*
 * uthreads.cpp
 *
 *  Created on: Apr 13, 2015
 *      Author: omerom88
 */

#include "uthreads.h"
#include "Thread.h"
#include "ReadyQueue.h"

#include <queue>
#include <map>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>

using namespace std;
#define SUCCESS 0
#define FAILURE -1
#define NONE 0
#define FACTOR 1000000
#define START 1
#define MAIN_ID 0

//globals containers:
ReadyQueue* gReadyQ;
priority_queue <int, vector<int>, greater<int> > gIdBank;
map <int,Thread*> gTrdBank;
map <int,Thread*> gTrdBlocks;

int gTotalQuantums;
int gQuantum;

//running thread
Thread* gRunner;
Thread* gNextRunner;

//timers parameters
itimerval timer;
sigset_t signalSet;


/* Function to block the timer */
void block()
{
	sigemptyset(&signalSet);
	sigaddset(&signalSet, SIGVTALRM);
	if(sigprocmask(SIG_BLOCK, &signalSet, NULL) == FAILURE)
	{
		cerr << "EINVAL The how argument is invalid" << endl;
		exit(SUCCESS);
	}
}


/* Function to block the timer */
void unblock()
{
	if(sigprocmask(SIG_UNBLOCK, &signalSet, NULL) == FAILURE)
	{
		cerr << "EINVAL The how argument is invalid" << endl;
		exit(SUCCESS);
	}
}


/* switch runners by clock pulse */
void switchTimer()
{
	block();

	int val =  sigsetjmp(gRunner->env, 1);
	if (val != NONE)
	{
		return;
	}

	int nextTid;
	//case there's no more threads in the ready list - keep the main running
	if (gReadyQ->QueuesSize() == NONE)
	{
		nextTid = MAIN_ID;
	}
	else
	{
		//return the runner to the ready list
		nextTid = gReadyQ->popReadyThread()->getTid();
		gReadyQ->pushThread(gRunner);
	}

	//choose next runner
	gTrdBank[nextTid]->setStats(RUNNIG);
	gTrdBank[nextTid]->increment();
	gTotalQuantums++;
	gRunner = gTrdBank[nextTid];

	// Start the timer for the next thread
	if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == FAILURE)
	{
		cerr << "system error:" << strerror(errno) << endl;
		unblock();
		exit(SUCCESS);
	}
	unblock();
	siglongjmp(gTrdBank[nextTid]->env, 1);
}


/* switch runners by block order */
void switchBlockTerminat(bool blockOrTerm)
{
	block();

	int val =  sigsetjmp(gRunner->env, 1);
	if (val != NONE)
	{
		return;
	}

	if(blockOrTerm)
	{
		//switch runners for blocking - add the runner to the blocked list
		gTrdBlocks[gRunner->getTid()] = gRunner;
		gRunner->setStats(BLOCKED);
	}
	else
	{
		//switch runners for terminating -  delete the old runner
		int oldRunnerTid = gRunner->getTid();
		gTrdBank.erase(oldRunnerTid);
		gTrdBank[oldRunnerTid] = NULL;
		gIdBank.push(oldRunnerTid);
		delete(gRunner);
	}

	//choose the next runner
	gNextRunner = gReadyQ->popReadyThread();
	gNextRunner->setStats(RUNNIG);
	gNextRunner->increment();
	gTotalQuantums++;
	gRunner = gNextRunner;

	if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == FAILURE)
	{
		cerr << "system error:" << strerror(errno) << endl;
		unblock();
		exit(1);
	}
	unblock();
	siglongjmp(gNextRunner->env, 1);
}


/* Function for handling clock pulse - when the clock gets a signal he calls
 * this function */
void timer_handler(int signal)
{
	switchTimer();
}


/* Function for initialize the clock */
void timerInit()
{
	int secs = floor(gQuantum/FACTOR);
	int micsecs = gQuantum - secs*FACTOR;

	signal(SIGVTALRM, timer_handler);
	timer.it_value.tv_sec = secs;  /* first time interval, seconds part */
	timer.it_value.tv_usec = micsecs; /* first time interval, microseconds part */
	timer.it_interval.tv_sec = secs;  /* following time intervals, seconds part */
	timer.it_interval.tv_usec = micsecs; /* following time intervals, microseconds part */
	setitimer(ITIMER_VIRTUAL, &timer, NULL);
}


/* Initialize the thread library *//* Initialize the thread library */
int uthread_init(int quantum_usecs)
{
	if(quantum_usecs <= NONE)
	{
		cerr << "thread library error: "
				"non-positive quantum usecs" << endl;
		return FAILURE;
	}
	gQuantum = quantum_usecs;
	Thread* main = new Thread(MAIN_ID, NULL, ORANGE, RUNNIG);
	gReadyQ = new ReadyQueue();
	main->increment();
	gTotalQuantums = START;
	//init the idBank
	for(int i = START; i < MAX_THREAD_NUM; i++)
	{
		gIdBank.push(i);
	}
	gTrdBank[MAIN_ID] = main;
	gRunner = main;
	timerInit();
	return SUCCESS;
}


/* Create a new thread whose entry point is f */
int uthread_spawn(void (*f)(void), Priority pr)
{
	block();
	//case we have more then MAX_THREAD_NUM threads
	if (gIdBank.empty())
	{
		unblock();
		cout << "thread library error: maximum threads" << endl;
		return FAILURE;
	}
	//find a new tid
	int newTid = gIdBank.top();
	gIdBank.pop();

	//Create a new thread
	Thread* newThread = new Thread(newTid, f , pr, READY);

	//add him to his ready queue, and to the thread bank
	gReadyQ->pushThread(newThread);
	gTrdBank[newTid] = newThread;
	unblock();
	return newTid;
}


/* Terminate a thread */
int uthread_terminate(int tid)
{
	block();
	//case of terminate the main
	if(tid == MAIN_ID)
	{
		while(gReadyQ->QueuesSize() != NONE)
		{
			delete(gReadyQ->popReadyThread());
		}

		delete(gReadyQ);
		delete(gRunner);
		exit(SUCCESS);
	}

	//case of terminate a non-exciting thread
	else if(tid > MAX_THREAD_NUM || tid < NONE)
	{
		unblock();
		cerr << "thread library error: "
				"can't terminate a non-exciting thread" << endl;
		return FAILURE;
	}
	//another case of terminate a non-exciting thread
	else if (gTrdBank[tid] == NULL)
	{
		unblock();
		cerr << "thread library error: "
				"can't terminate a non-exciting thread" << endl;
		return FAILURE;
	}

	Status trdStats = gTrdBank[tid]->getStats();

	//case of terminate a blocked thread or a ready thread
	if (trdStats == BLOCKED || trdStats == READY)
	{
		if (trdStats == BLOCKED)
		{
			gTrdBlocks.erase(tid);
			gTrdBlocks[tid] = NULL;
		}
		else
		{
			gReadyQ->deleteFromQueue(gTrdBank[tid]);
		}
		gTrdBank.erase(tid);
		gTrdBank[tid] = NULL;
		gIdBank.push(tid);
		unblock();
		return SUCCESS;
	}

	//case of thread terminates itself
	else if(trdStats == RUNNIG)
	{
		switchBlockTerminat(false);
		return SUCCESS;
	}
	return FAILURE;
}


/* Suspend a thread */
int uthread_suspend(int tid)
{
	block();
	//case of blocking the main
	if (tid == MAIN_ID)
	{
		unblock();
		cerr << "thread library error: can't block the main thread" << endl ;
		return FAILURE;
	}

	//case of block a non-exciting thread
	if(tid > MAX_THREAD_NUM || tid < NONE)
	{
		unblock();
		cerr << "thread library error: "
				"can't block a non-exciting thread" << endl;
		return FAILURE;
	}
	//another case of block a non-exciting thread
	else if (gTrdBank[tid] == NULL)
	{
		unblock();
		cerr << "thread library error: "
				"can't block a non-exciting thread" << endl;
		return FAILURE;
	}
	Status trdStats = gTrdBank[tid]->getStats();

	//case of the thread was in blocked list
	if(trdStats == BLOCKED)
	{
		unblock();
		return SUCCESS;
	}

	//case of the thread was in ready list
	if(trdStats == READY)
	{
		gTrdBank[tid]->setStats(BLOCKED);
		gTrdBlocks[tid] = gTrdBank[tid];
		gReadyQ->deleteFromQueue(gTrdBank[tid]);
		unblock();
		return SUCCESS;
	}

	//case of the thread was running
	if(trdStats == RUNNIG)
	{
		switchBlockTerminat(true);
	}
	return -1;
}


/* Resume a thread */
int uthread_resume(int tid)
{
	block();
	//case of resume a non-exciting thread
	if(tid > MAX_THREAD_NUM || tid < NONE)
	{
		unblock();
		cerr << "thread library error: "
				"can't resume a non-exciting thread" << endl;
		return FAILURE;
	}
	//another case of resume a non-exciting thread
	else if (gTrdBank[tid] == NULL)
	{
		unblock();
		cerr << "thread library error: "
				"can't resume a non-exciting thread" << endl;
		return FAILURE;
	}
	Status trdStats = gTrdBank[tid]->getStats();

	//case of thread from the ruuning list or ready list
	if (trdStats == RUNNIG || trdStats == READY)
	{
		unblock();
		return SUCCESS;
	}

	//case of thread is blocked
	else if (trdStats == BLOCKED)
	{
		gTrdBlocks.erase(tid);
		gReadyQ->pushThread(gTrdBank[tid]);
		gTrdBank[tid]->setStats(READY);
		unblock();
		return SUCCESS;
	}
	return FAILURE;
}


/* Get the id of the calling thread */
int uthread_get_tid()
{
	return gRunner->getTid();
}


/* Get the total number of library quantums */
int uthread_get_total_quantums()
{
	return gTotalQuantums;
}


/* Get the number of thread quantums */
int uthread_get_quantums(int tid)
{
	//case of a non-exciting thread
	if(tid < NONE || gTrdBank[tid] == NULL)
	{
		unblock();
		cerr << "thread library error: "
				"non-exciting thread" << endl;
		return FAILURE;
	}
	unblock();
	return gTrdBank[tid]->getQuantumCnt();
}
