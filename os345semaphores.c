// os345semaphores.c - OS Semaphores	06/21/2020
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the BYU CS345 projects.      **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>
#include <assert.h>
#include <execinfo.h>

#include "os345.h"


extern TCB tcb[];							// task control block
extern int curTask;							// current task #

extern int superMode;						// system mode
extern Semaphore* semaphoreList;			// linked list of active semaphores

extern DeltaClock *deltaClock;
extern int deltaClockCount;
extern int P2_listTasks(int, char**);
extern long swapCount;


// **********************************************************************
// **********************************************************************
// signal semaphore
//
//	if semaphore has blocked tasks, dequeue the highest priority task and enqueue it in the ready queue
//      if binary semaphore and there are still blocked tasks after signaling, mark s->state as 0
//      if counting semaphore, increment s->state
//  if semaphore has no blocked tasks, then signal the semaphore and return
//
void semSignal(Semaphore* s)
{
	int i;
	// assert there is a semaphore and it is a legal type
	assert("semSignal Error");
    if (!s) {
        void* callstack[128];
        int frames = backtrace(callstack, 128);
        char** strs = backtrace_symbols(callstack, frames);
        for (i = 0; i < frames; ++i) {
            printf("\n%s", strs[i]);
        }
        printf("\n");
        free(strs);
        assert(s);
    }
    assert((s->type == 0) || (s->type == 1));

	// check semaphore type
	if (s->type == 0)
	{
		// binary semaphore
		// look through tasks for one suspended on this semaphore

        // dequeue the highest priority task
        // enqueue it back on the main stack
        // when task is put into ready queue, clear the event it is waiting on and set state to ready

        int tid = deQ(s->pq, -1);
        // if deQ returns -1, then mark the semaphore as signaled
        if (tid == -1) {
            s->state = 1;
        }
        // if deQ returns an actual tid, then mark the semaphore as 0 still
        else {
            enQ(rq, tid, tcb[tid].priority);
            tcb[tid].event = 0;			// clear event pointer
            tcb[tid].state = S_READY;	    // unblock task
            s->state = 0;
        }
		// nothing waiting on semaphore, go ahead and just signal
		if (!superMode) swapTask();
		return;
	}
	else
	{
		// counting semaphore
		// ?? implement counting semaphore
        // counting semaphore increments every time it is signaled
        s->state += 1;
        if (s->state > 0) return;

        int tid = deQ(s->pq, -1);
        if (tid != -1) {
            enQ(rq, tid, tcb[tid].priority);
            tcb[tid].event = 0;			// clear event pointer
            tcb[tid].state = S_READY;	    // unblock task
        }

        if (!superMode) swapTask();
        return;
	}
} // end semSignal



// **********************************************************************
// **********************************************************************
// wait on semaphore
//
//	if semaphore is signaled, return immediately
//	else block task
//
int semWait(Semaphore* s)
{
	assert("semWait Error" && s);												// assert semaphore
	assert("semWait Error" && ((s->type == 0) || (s->type == 1)));	            // assert legal type
	assert("semWait Error" && !superMode);								        // assert user mode

	// check semaphore type
	if (s->type == 0)
	{
        if (s->state == 0) {
            int tid = deQ(rq, curTask);
            if (tid != -1) {
                tcb[curTask].event = s;
                tcb[curTask].state = S_BLOCKED;
                enQ(s->pq, tid, tcb[tid].priority);
            }

            if (!superMode) swapTask();
            return 1;
        } else {
            s->state = 0;
        }

        if (!superMode) swapTask();
        return 0;
	}
	else
	{
		// counting semaphore
        s->state -= 1;
        if (s->state < 0) {
            int tid = deQ(rq, curTask);
            if (tid != -1) {
                tcb[curTask].event = s;
                tcb[curTask].state = S_BLOCKED;
                enQ(s->pq, tid, tcb[tid].priority);
            }
        }
        if (!superMode) swapTask();
        return 1;
	}
} // end semWait



// **********************************************************************
// **********************************************************************
// try to wait on semaphore
//
//	if semaphore is signaled, return 1
//	else return 0
//
int semTryLock(Semaphore* s)
{
	assert("semTryLock Error" && s);											// assert semaphore
	assert("semTryLock Error" && ((s->type == 0) || (s->type == 1)));	        // assert legal type
	assert("semTryLock Error" && !superMode);									// assert user mode

	// check semaphore type
    // can use semTryLock to call semWait or can use it to tell if a semaphore has been blocked
	if (s->type == 0)
	{
        return s->state;
	}
	else
	{
		// counting semaphore
		// ?? implement counting semaphore
        return s->state > 0;
	}
} // end semTryLock


// **********************************************************************
// **********************************************************************
// Create a new semaphore.
// Use heap memory (malloc) and link into semaphore list (Semaphores)
// 	name = semaphore name
//		type = binary (0), counting (1)
//		state = initial semaphore state
// Note: memory must be released when the OS exits.
//
Semaphore* createSemaphore(char* name, int type, int state)
{
	Semaphore* sem = semaphoreList;
	Semaphore** semLink = &semaphoreList;

	// assert semaphore is binary or counting
	assert("createSemaphore Error" && ((type == 0) || (type == 1)));	// assert type is validate

	// look for duplicate name
	while (sem)
	{
		if (!strcmp(sem->name, name))
		{
			printf("\nSemaphore %s already defined", sem->name);

			// ?? What should be done about duplicate semaphores ??
			// semaphore found - change to new state
			sem->type = type;				// 0=binary, 1=counting
			sem->state = state;				// initial semaphore state
			sem->taskNum = curTask;			// set parent task #
			return sem;
		}
		// move to next semaphore
		semLink = (Semaphore**)&sem->semLink;
		sem = (Semaphore*)sem->semLink;
	}

	// allocate memory for new semaphore
	sem = (Semaphore*)malloc(sizeof(Semaphore));

	// set semaphore values
	sem->name = (char*)malloc(strlen(name)+1);
	strcpy(sem->name, name);				                // semaphore name
	sem->type = type;							            // 0=binary, 1=counting
	sem->state = state;						                // initial semaphore state
	sem->taskNum = curTask;					                // set parent task #
    sem->pq = (int*)malloc(MAX_TASKS * sizeof(int));   // allocate memory for priority queue
    sem->pq[0] = 0;                                         // pq[0] keeps track of the size of the queue

	// prepend to semaphore list
	sem->semLink = (struct semaphore*)semaphoreList;
	semaphoreList = sem;						// link into semaphore list
	return sem;									// return semaphore pointer
} // end createSemaphore



// **********************************************************************
// **********************************************************************
// Delete semaphore and free its resources
//
bool deleteSemaphore(Semaphore** semaphore)
{
	Semaphore* sem = semaphoreList;
	Semaphore** semLink = &semaphoreList;

	// assert there is a semaphore
	assert("deleteSemaphore Error" && *semaphore);

	// look for semaphore
	while(sem)
	{
		if (sem == *semaphore)
		{
			// semaphore found, delete from list, release memory
			*semLink = (Semaphore*)sem->semLink;

			// free the name array before freeing semaphore
			printf("\ndeleteSemaphore(%s)", sem->name);

			// ?? free all semaphore memory
			// ?? What should you do if there are tasks in this
			//    semaphores blocked queue????
            while(sem->pq[0] > 0) {
                int tid = deQ(sem->pq, -1);
                enQ(rq, tid, tcb[tid].priority);
            }
			free(sem->name);
			free(sem);

			return TRUE;
		}
		// move to next semaphore
		semLink = (Semaphore**)&sem->semLink;
		sem = (Semaphore*)sem->semLink;
	}

	// could not delete
	return FALSE;
} // end deleteSemaphore
