// os345p2.c - 5 state scheduling	06/21/2020
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
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
#include <assert.h>
#include <time.h>
#include "os345.h"
#include "os345signals.h"

#define my_printf	printf

// ***********************************************************************
// project 2 variables
static Semaphore* s1Sem;					// task 1 semaphore
static Semaphore* s2Sem;					// task 2 semaphore
extern Semaphore* tics10sec;

extern TCB tcb[];								// task control block
extern int curTask;							// current task #
extern Semaphore* semaphoreList;			// linked list of active semaphores
extern jmp_buf reset_context;				// context of kernel stack

// ***********************************************************************
// project 2 functions and tasks

int signalTask(int, char**);
int ImAliveTask(int, char**);
int TimerTask(int, char**);

// ***********************************************************************
// ***********************************************************************
// project2 command
int P2_main(int argc, char* argv[])
{
	static char* s1Argv[] = {"signal1", "s1Sem"};
	static char* s2Argv[] = {"signal2", "s2Sem"};
	static char* aliveArgv[] = {"I'm Alive", "3"};
    static char* timerTask[] = {"timer1", "tics10sec"};

	printf("\nStarting Project 2");
	SWAP;

	// start tasks looking for sTask semaphores
    printf("\nCreating tasks");
    createTask("timer1", TimerTask, 10, 2, timerTask);
    createTask("timer2", TimerTask, 10, 2, timerTask);
    createTask("timer3", TimerTask, 10, 2, timerTask);
    createTask("timer4", TimerTask, 10, 2, timerTask);
    createTask("timer5", TimerTask, 10, 2, timerTask);
    createTask("timer6", TimerTask, 10, 2, timerTask);
    createTask("timer7", TimerTask, 10, 2, timerTask);
    createTask("timer8", TimerTask, 10, 2, timerTask);
    createTask("timer9", TimerTask, 10, 2, timerTask);
    printf("\nFinished creating timer tasks");

	createTask("signal1",				// task name
					signalTask,				// task
					VERY_HIGH_PRIORITY,	// task priority
					2,							// task argc
					s1Argv);					// task argument pointers

	createTask("signal2",				// task name
					signalTask,				// task
					VERY_HIGH_PRIORITY,	// task priority
					2,							// task argc
					s2Argv);					// task argument pointers

	createTask("I'm Alive",				// task name
					ImAliveTask,			// task
					LOW_PRIORITY,			// task priority
					2,							// task argc
					aliveArgv);				// task argument pointers

	createTask("I'm Alive",				// task name
					ImAliveTask,			// task
					LOW_PRIORITY,			// task priority
					2,							// task argc
					aliveArgv);				// task argument pointers
	return 0;
} // end P2_project2



// ***********************************************************************
// ***********************************************************************
// list tasks command
int P2_listTasks(int argc, char* argv[])
{
	int i;

    PQueue combinedKeys = (int*)malloc(MAX_TASKS * sizeof(int));;
    for (i = 0; i < MAX_TASKS; i++) {
        if (tcb[i].name) {
            enQ(combinedKeys, i, tcb[i].priority);
        }
    }
    while (1) {
        int tid = deQ(combinedKeys, -1);
        if (tid == -1) {
            break;
        }
        printf("\n%4d/%-4d%20s%4d  ", tid, tcb[tid].parent,
               tcb[tid].name, tcb[tid].priority);
        if (tcb[tid].signal & mySIGSTOP) my_printf("Paused");
        else if (tcb[tid].state == S_NEW) my_printf("New");
        else if (tcb[tid].state == S_READY) my_printf("Ready");
        else if (tcb[tid].state == S_RUNNING) my_printf("Running");
        else if (tcb[tid].state == S_BLOCKED)
            my_printf("Blocked    %s",
                      tcb[tid].event->name);
        else if (tcb[tid].state == S_EXIT) my_printf("Exiting");
        swapTask();
    }
	return 0;
} // end P2_listTasks



// ***********************************************************************
// ***********************************************************************
// list semaphores command
//
int match(char* mask, char* name)
{
   int i,j;

   // look thru name
	i = j = 0;
	if (!mask[0]) return 1;
	while (mask[i] && name[j])
   {
		if (mask[i] == '*') return 1;
		if (mask[i] == '?') ;
		else if ((mask[i] != toupper(name[j])) && (mask[i] != tolower(name[j]))) return 0;
		i++;
		j++;
   }
	if (mask[i] == name[j]) return 1;
   return 0;
} // end match

int P2_listSems(int argc, char* argv[])				// listSemaphores
{
	Semaphore* sem = semaphoreList;
    printf("\n%20s  %5s  %5s  %5s", "name", "type", "state", "name");
	while(sem)
	{
		if ((argc == 1) || match(argv[1], sem->name))
		{
			printf("\n%20s  %5c  %5d  %5s", sem->name, (sem->type?'C':'B'), sem->state,
	  					tcb[sem->taskNum].name);
		}
		sem = (Semaphore*)sem->semLink;
	}
	return 0;
} // end P2_listSems



// ***********************************************************************
// ***********************************************************************
// reset system
int P2_reset(int argc, char* argv[])						// reset
{
	longjmp(reset_context, POWER_DOWN_RESTART);
	// not necessary as longjmp doesn't return
	return 0;

} // end P2_reset



// ***********************************************************************
// ***********************************************************************
// kill task

int P2_killTask(int argc, char* argv[])			// kill task
{
	int taskId = INTEGER(argv[1]);				// convert argument 1

	if (taskId > 0) printf("\nKill Task %d", taskId);
	else printf("\nKill All Tasks");

	// kill task
	if (killTask(taskId)) printf("\nkillTask Error!");

	return 0;
} // end P2_killTask



// ***********************************************************************
// ***********************************************************************
// signal command
void sem_signal(Semaphore* sem)		// signal
{
	if (sem)
	{
		printf("\nSignal %s", sem->name);
		SEM_SIGNAL(sem);
	}
	else my_printf("\nSemaphore not defined!");
	return;
} // end sem_signal



// ***********************************************************************
int P2_signal1(int argc, char* argv[])		// signal1
{
	SEM_SIGNAL(s1Sem);
	return 0;
} // end signal

int P2_signal2(int argc, char* argv[])		// signal2
{
	SEM_SIGNAL(s2Sem);
	return 0;
} // end signal



// ***********************************************************************
// ***********************************************************************
// signal task
//
#define COUNT_MAX	5
//
int signalTask(int argc, char* argv[])
{
	int count = 0;					// task variable

	// create a semaphore
	Semaphore** mySem = (!strcmp(argv[1], "s1Sem")) ? &s1Sem : &s2Sem;
	*mySem = createSemaphore(argv[1], 0, 0);

	// loop waiting for semaphore to be signaled
	while(count < COUNT_MAX)
	{
		SEM_WAIT(*mySem);			// wait for signal
		printf("\n%s  Task[%d], count=%d", tcb[curTask].name, curTask, ++count);
	}
	return 0;						// terminate task
} // end signalTask



// ***********************************************************************
// ***********************************************************************
// I'm alive task
int ImAliveTask(int argc, char* argv[])
{
	int i;							// local task variable
	while (1)
	{
		printf("\n(%d) I'm Alive!", curTask);
		for (i=0; i<100000; i++) swapTask();
	}
	return 0;						// terminate task
} // end ImAliveTask

int TimerTask(int argc, char* argv[]) {
    char* svtime[20];
    svtime[0] = '\0';

    while (1) {
        SEM_WAIT(tics10sec);
        printf("\nCurrent Task:\t%d\tCurrent Time:\t%s", curTask, myTime(svtime));
    }
    return 0;
}



// **********************************************************************
// **********************************************************************
// read current time
//
char* myTime(char* svtime)
{
	time_t cTime;						// current time

	time(&cTime);						// read current time
	strcpy(svtime, asctime(localtime(&cTime)));
	svtime[strlen(svtime)-1] = 0;		// eliminate nl at end
	return svtime;
} // end myTime

int enQ(PQueue q, TID tid, Priority p) {
    // get the length of q
    // create the int32 representation of TID/Priority (int16/int16)
    assert(tid <= 32767);
    assert(p <= 32767);
    q[0]++;

    // combines the ints into one
    int32_t combined = (int32_t) (((int32_t) p << 16) | ((int32_t) tid));
    // puts the new key at the end of the queue
    q[q[0]] = combined;

    // gets the number of items (including the newest one) in the queue and puts it in index
    int32_t index = q[0];
    // loop breaks if index == 1, since the task is already at the top of the queue and can't replace the counter
    while (1) {
        if (index == 1) {
            break;
        }
        if (p <= getPriority(q[index - 1])) {
            break;
        }
        int32_t temp = q[index - 1];
        q[index - 1] = q[index];
        q[index--] = temp;
    }

    return combined;
}

int deQ(PQueue q, TID tid) {
    size_t index = 0;
    int16_t returnTid = -1;
    if (q[0] == 0) {
        return -1;
    }

    // if tid == -1, then remove the highest priority task by cycling
    // through the queue and comparing all the priorities
    if (tid == -1) {
        returnTid = (int16_t) q[1];
        index = 1;

        // will always remove the highest priority task unless the priority queue is empty
        decrementPQueue(q, index);
    } else {
        // if tid != -1, then find the specific task by cycling through every task in the queue
        for (size_t i = 1; i <= q[0]; ++i) {
            // if the tid is found, then set the index marker and the returnTid and break
            // don't need to look at every item in the queue like the other one
            if (getTid(q[i]) == tid) {
                index = i;
                returnTid = (int16_t) q[i];
                break;
            }
        }
        // if the task is found, then move the queue along
        if (returnTid != -1) {
            decrementPQueue(q, index);
        }
    }

    return returnTid;
}

void decrementPQueue(PQueue q, size_t index) {
    for (size_t i = index; i < q[0]; ++i) {
        q[i] = q[i + 1];
    }
    q[0] -= 1;
}

int16_t getPriority(int32_t combinedKey) {
    return (int16_t) (combinedKey >> 16);
}

int16_t getTid(int32_t combinedKey) {
    return (int16_t) combinedKey;
}