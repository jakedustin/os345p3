// os345tasks.c - OS create/kill task	06/21/2020
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

#include "os345.h"
#include "os345signals.h"
//#include "os345config.h"

extern TCB tcb[];   // task control block
extern int curTask; // current task #

extern int superMode;                  // system mode
extern Semaphore *semaphoreList;       // linked list of active semaphores
extern Semaphore *taskSems[MAX_TASKS]; // task semaphore

// **********************************************************************
// **********************************************************************
// create task
int createTask(char *name,                // task name
               int (*task)(int, char **), // task address
               int priority,              // task priority
               int argc,                  // task argument count
               char *argv[])              // task argument pointers
{
    int tid;

    // find an open tcb entry slot
    for (tid = 0; tid < MAX_TASKS; tid++)
    {
        if (tcb[tid].name == 0)
        {
            char buf[8];

            // create task semaphore
            if (taskSems[tid])
                deleteSemaphore(&taskSems[tid]);
            sprintf(buf, "task%d", tid);
            taskSems[tid] = createSemaphore(buf, 0, 0);
            taskSems[tid]->taskNum = 0; // assign to shell

            // copy task name
            tcb[tid].name = (char *)malloc(strlen(name) + 1);
            strcpy(tcb[tid].name, name);

            // set task address and other parameters
            tcb[tid].task = task;         // task address
            tcb[tid].state = S_NEW;       // NEW task state
            tcb[tid].priority = priority; // task priority
            tcb[tid].parent = curTask;    // parent
            tcb[tid].argc = argc;         // argument count

            // malloc new argv parameters
            tcb[tid].argv = argv; // argument pointers
            tcb[tid].argv = (char **)(malloc(argc * sizeof(char *)));
            for (int i = 0; i < argc; ++i)
            {
                size_t currentStringLength = strlen(argv[i]);
                tcb[tid].argv[i] = (char *)(malloc(currentStringLength + 1));

                strcpy(tcb[tid].argv[i], argv[i]);
            }

            tcb[tid].event = 0;   // suspend semaphore
            tcb[tid].RPT = 0;     // root page table (project 5)
            tcb[tid].cdir = CDIR; // inherit parent cDir (project 6)

            // define task signals
            createTaskSigHandlers(tid);

            // Each task must have its own stack and stack pointer.
            tcb[tid].stack = malloc(STACK_SIZE * sizeof(int));

            // inserting task into "ready" queue
            enQ(rq, tid, tcb[tid].priority);

            if (tid)
                swapTask(); // do context switch (if not cli)
            return tid;     // return tcb index (curTask)
        }
    }
    // tcb full!
    return -1;
} // end createTask

// **********************************************************************
// **********************************************************************
// kill task
//
//	taskId == -1 => kill all non-shell tasks
//
static void exitTask(int taskId);
int killTask(int taskId)
{
    if (taskId != 0) // don't terminate shell
    {
        if (taskId < 0) // kill all tasks
        {
            int tid;
            for (tid = 1; tid < MAX_TASKS; tid++)
            {
                if (tcb[tid].name)
                {
                    exitTask(tid);
                }
            }
        }
        else
        {
            // terminate individual task
            if (!tcb[taskId].name)
                return 1;
            exitTask(taskId); // kill individual task
        }
    }
    if (!superMode)
        SWAP;
    return 0;
} // end killTask

static void exitTask(int taskId)
{
    assert("exitTaskError" && tcb[taskId].name);

    // 1. find task in system queue
    // 2. if blocked, unblock (handle semaphore)
    // 3. set state to exit

    // ?? add code here
    if (tcb[taskId].state == S_BLOCKED)
    {
        deQ(tcb[taskId].event->pq, taskId);
        if (tcb[taskId].event->type == 1 && tcb[taskId].event->state < 0)
            tcb[taskId].event->state += 1;
        enQ(rq, taskId, tcb[taskId].priority);
    }

    tcb[taskId].state = S_EXIT; // EXIT task state
    return;
} // end exitTask

// **********************************************************************
// system kill task
//
int sysKillTask(int taskId)
{
//    printf("\nattempting to kill task %d", taskId);
//    fflush(stdout);
//    printf("\ntcb[taskId].name:\t%s", tcb[taskId].name);
//    fflush(stdout);
    Semaphore *sem = semaphoreList;
    Semaphore **semLink = &semaphoreList;

    // assert that you are not pulling the rug out from under yourself!
    assert("sysKillTaskError -> task" && tcb[taskId].name);
    assert("sysKillTaskError -> superMode" && superMode);
    assert("sysKillTask Error" && tcb[taskId].name && superMode);
    printf("\nKill Task %s", tcb[taskId].name);

    // signal task terminated
    semSignal(taskSems[taskId]);

    // look for any semaphores created by this task
    while (sem = *semLink)
    {
        if (sem->taskNum == taskId)
        {
            // semaphore found, delete from list, release memory
            deleteSemaphore(semLink);
        }
        else
        {
            // move to next semaphore
            semLink = (Semaphore **)&sem->semLink;
        }
    }

    for (int i = 0; i < tcb[taskId].argc; ++i)
    {
        free(tcb[taskId].argv[i]);
    }
    free(tcb[taskId].argv);

    deQ(rq, taskId);
    tcb[taskId].name = 0; // release tcb slot
    return 0;
} // end sysKillTask
