// ***********************************************************************
// project 3 variables

#include <stdlib.h>
#include <stdio.h>
#include "os345park.h"
#include "os345.h"

// Jurassic Park
extern JPARK myPark;
extern DeltaClock *deltaClock;
extern int deltaClockCount;
extern TCB tcb[];
int timeTaskID;
int dcMonitorTaskID;
int carIdHolder;
Semaphore **event;

extern Semaphore *parkMutex;            // protect park access
extern Semaphore *fillSeat[NUM_CARS];   // (signal) seat ready to fill
extern Semaphore *seatFilled[NUM_CARS]; // (wait) passenger seated
extern Semaphore *rideOver[NUM_CARS];   // (signal) ride over

extern Semaphore *dcChange;
Semaphore *mailbox;

// ******************************************************************* //
//                         RESOURCE SEMAPHORES                         //
// ******************************************************************* //
Semaphore *tickets;

// ******************************************************************* //
//                          MUTEX SEMAPHORES                           //
// ******************************************************************* //
Semaphore *getTicketMutex;
Semaphore *needDriverMutex;

// ******************************************************************* //
//                          SIGNAL SEMAPHORES                          //
// ******************************************************************* //
Semaphore *wakeupDriver;
Semaphore *driverDone;
Semaphore *seatTaken;
Semaphore *passengerSeated;
Semaphore *driverInCar;
Semaphore *goodToGo;
Semaphore *needTicket;
Semaphore *ticketReady;
Semaphore *buyTicket;

// ***********************************************************************
// project 3 functions and tasks
void CL3_project3(int, char **);

void CL3_dc(int, char **);

int insertIntoDeltaClock(int, Semaphore *);

int popDeltaClock();

int dcMonitorTask(int, char **);

int timeTask(int, char **);

int carTask(int, char **);

int visitorTask(int, char **);

int driverTask(int, char **);

extern int P3_tdc(int, char **);

extern void printDeltaClock();

// ***********************************************************************
// ***********************************************************************
// project3 command
int P3_main(int argc, char *argv[])
{
    char buf[32];
    char **newArgv = (char **)malloc(2 * sizeof(char *));
    parkMutex = NULL;

    // start park
    sprintf(buf, "jurassicPark");
    newArgv[0] = buf;
    createTask(buf,          // task name
               jurassicTask, // task
               MED_PRIORITY, // task priority
               1,            // task count
               newArgv);     // task argument

    // wait for park to get initialized...
    while (!parkMutex)
        SWAP;
    printf("\nStart Jurassic Park...");
    fflush(stdout);
    SWAP;
    printf("\nSetting park variables");
    fflush(stdout);
    myPark.numInCarLine = myPark.numInPark = 4;
    SWAP;

    //?? create car, driver, and visitor tasks here
    printf("\nCreating car tasks - NUM_CARS = %d", NUM_CARS);
    fflush(stdout);
    for (int i = 0; i < NUM_CARS; ++i)
    {
        printf("\nCreating car #%d", i);
        fflush(stdout);
        sprintf(buf, "Car #%d", i);
        printf("\nbuf = %s", buf);
        fflush(stdout);
        SWAP;
        newArgv[0] = i;
        SWAP;
        createTask(buf, carTask, MED_PRIORITY, 1, newArgv);
        SWAP;
    }

    return 0;
} // end project3

// ************************************************************************ //
// ******************************* CAR TASK ******************************* //
// ************************************************************************ //
int carTask(int argc, char *argv[])
{
    printf("\nargv[0]: %s", argv[0]);
    fflush(stdout);
    // int carId = atoi(argv[0]);
    // SWAP;
    while (1)
    {
        printf("\nBeginning of car task while loop");
        fflush(stdout);
        SEM_WAIT(rideOver[argc]);
        printf("\nSignaled rideOver semaphore.");
        fflush(stdout);
        SWAP;
    }

    return 0;
}

// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char *argv[])
{
    printf("\nDelta Clock");
    int i;
    for (i = 0; i < deltaClockCount; i++)
    {
        printf("\n%4d%4d  %-20s", i, deltaClock[i].time, deltaClock[i].sem->name);
    }
    return 0;
} // end CL3_dc

// insert into delta clock
// returns the index of the insertion
int insertIntoDeltaClock(int time, Semaphore *semaphore)
{
    printf("\nInserting new item into delta clock: %d\t\t%s", time, semaphore->name);
    printDeltaClock();
    // if the delta clock is empty, then insert the item at the front and increment the clock's size
    if (deltaClockCount == 0)
    {
        deltaClock = (DeltaClock *)malloc(MAX_TASKS * sizeof(DeltaClock));
        for (int i = 0; i < MAX_TASKS; ++i)
        {
            deltaClock[i].time = 0;
            deltaClock[i].sem = NULL;
        }
        DeltaClock x = {time, semaphore};
        deltaClock[0] = x;
        deltaClockCount = 1;

        return 0;
    }

    // otherwise, iterate through the clock and find where the item belongs
    for (int i = 0; i <= deltaClockCount; ++i)
    {
        if (time < deltaClock[i].time || i == deltaClockCount)
        {
            for (int j = deltaClockCount; j > i; j--)
            {
                deltaClock[j] = deltaClock[j - 1];
                if (j == i + 1)
                {
                    deltaClock[j].time -= time;
                }
            }
            deltaClock[i].time = time;
            deltaClock[i].sem = semaphore;
            deltaClockCount += 1;
            break;
        }
        else
        {
            time -= deltaClock[i].time;
        }
    }
    return 0;
}

int popDeltaClock()
{
    deltaClockCount -= 1;
    for (int i = 0; i < deltaClockCount; ++i)
    {
        deltaClock[i] = deltaClock[i + 1];
    }

    SEM_SIGNAL(dcChange);
    return 0;
}

// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// display all pending events in the delta clock list
void printDeltaClock(void)
{
    int i;
    for (i = 0; i < deltaClockCount; i++)
    {
        printf("\n%4d%4d  %-20s", i, deltaClock[i].time, deltaClock[i].sem->name);
    }
    return;
}

// ***********************************************************************
// test delta clock
int P3_tdc(int argc, char *argv[])
{
    createTask("DC Test",     // task name
               dcMonitorTask, // task
               10,            // task priority
               argc,          // task arguments
               argv);

    timeTaskID = createTask("Time",   // task name
                            timeTask, // task
                            10,       // task priority
                            argc,     // task arguments
                            argv);
    return 0;
} // end P3_tdc

// ***********************************************************************
// monitor the delta clock task
int dcMonitorTask(int argc, char *argv[])
{
    event = (Semaphore **)malloc(100 * sizeof(Semaphore *));
    int i, flg;
    char buf[32];
    // create some test times for event[0-9]
    int ttime[10] = {
        90, 300, 50, 170, 340, 300, 50, 300, 40, 110};

    for (i = 0; i < 10; i++)
    {
        sprintf(buf, "event[%d]", i);
        event[i] = createSemaphore(buf, BINARY, 0);
        insertIntoDeltaClock(ttime[i], event[i]);
    }
    printDeltaClock();

    dcChange = createSemaphore("DCChange", BINARY, 0);
    while (deltaClockCount > 0)
    {
        SEM_WAIT(dcChange)
        flg = 0;
        for (i = 0; i < 10; i++)
        {
            if (event[i]->state == 1)
            {
                printf("\n  event[%d] signaled", i);
                event[i]->state = 0;
                flg = 1;
            }
        }
        if (flg)
            printDeltaClock();
    }
    printf("\nNo more events in Delta Clock");

    // kill dcMonitorTask
    tcb[timeTaskID].state = S_EXIT;
    return 0;
} // end dcMonitorTask

extern Semaphore *tics1sec;

// ********************************************************************************************
// display time every tics1sec
int timeTask(int argc, char *argv[])
{
    char svtime[64]; // ascii current time
    while (1)
    {
        SEM_WAIT(tics1sec)
        printf("\nTime = %s", myTime(svtime));
    }
    return 0;
} // end timeTask