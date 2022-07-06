// os345p3.c - Jurassic Park 07/27/2020
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
#include <time.h>
#include <assert.h>
#include "os345.h"
#include "os345park.h"

// ***********************************************************************
// project 3 variables

// Jurassic Park
extern JPARK myPark;
extern DeltaClock *deltaClock;
extern int deltaClockCount;
extern TCB tcb[];
int timeTaskID;
int dcMonitorTaskID;
int carIdHolder;
Semaphore **event;

extern Semaphore *parkMutex;                        // protect park access
extern Semaphore *fillSeat[NUM_CARS];               // (signal) seat ready to fill
extern Semaphore *seatFilled[NUM_CARS];             // (wait) passenger seated
extern Semaphore *rideOver[NUM_CARS];               // (signal) ride over

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

int visitorTask(int, char**);

int driverTask(int, char**);

extern int P3_tdc(int, char **);

extern void printDeltaClock();


// ***********************************************************************
// ***********************************************************************
// project3 command
int P3_main(int argc, char *argv[]) {
    char buf[32];
    char *newArgv[2];
    parkMutex = NULL;
    // TEMP
    myPark.numInCarLine = myPark.numInPark = 4;

    // start park
    sprintf(buf, "jurassicPark");
    newArgv[0] = buf;
    createTask(buf,                         // task name
               jurassicTask,                // task
               MED_PRIORITY,                // task priority
               1,                           // task count
               newArgv);                    // task argument

    // wait for park to get initialized...
    while (!parkMutex) SWAP;
    printf("\nStart Jurassic Park...");

    // create semaphores
    // MUTEX CREATION
    sprintf(buf, "getTicketMutex");
    getTicketMutex = createSemaphore(buf, BINARY, 1);
    sprintf(buf, "needDriverMutex");
    needDriverMutex = createSemaphore(buf, BINARY, 1);

    //?? create car, driver, and visitor tasks here
    for (int carId = 0; carId < NUM_CARS; ++carId) {
        sprintf(buf, "car[%d]", carId);
        newArgv[0] = carId;
        createTask(buf, carTask, MED_PRIORITY, 1, newArgv);
    }

    for (int driverId = 0; driverId < NUM_DRIVERS; ++driverId) {
        sprintf(buf, "driver[%d]", driverId);
        newArgv[0] = driverId;
        createTask(buf, driverTask, MED_PRIORITY, 1, newArgv);
    }

    for (int visitorId = 0; visitorId < NUM_VISITORS; ++visitorId) {
        sprintf(buf, "visitor[%d]", visitorId);
        newArgv[0] = visitorId;
        createTask(buf, visitorTask, MED_PRIORITY, 1, newArgv);
    }

    return 0;
} // end project3

int carTask(int argc, char *argv[]) {
    int carId = atoi(argv[0]);
    printf("\nCar task");
    // main loop
    while (1) {
        for (int i = 0; i < 3; ++i) {
            carIdHolder = carId;
            SEM_WAIT(fillSeat[carId]);          SWAP;
            // TODO: get passenger
            // SEM_SIGNAL(getPassenger);           SWAP;
            // NOTE: visitor signals that they are in the seat
            // SEM_WAIT(seatTaken);                SWAP;
            // NOTE: visitor is waiting on the car to say that the car has seated
            // SEM_SIGNAL(passengerSeated);        SWAP;

            if (i == 2) {
                // NOTE: needDriverMutex just prevents other cars from attempting to get a driver and do all this stuff
                //  at the same time as the given car
                SEM_WAIT(needDriverMutex);          SWAP;
                {
                    SEM_SIGNAL(wakeupDriver);       SWAP;
//                     TODO: create semaphore and put in mailbox
//                     driver will check the mailbox and then check the global int to get the car number
//                     driver will wait on the semaphore until car signals and the handshake is done
                    SEM_WAIT(driverInCar);         SWAP;
                    SEM_SIGNAL(goodToGo);          SWAP;
                }
                SEM_SIGNAL(needDriverMutex);        SWAP;
            }
            SEM_SIGNAL(seatFilled[carId]);          SWAP;
        }

        SEM_WAIT(rideOver[carId]);              SWAP;
        // TODO: create driverDone semaphore for driver
        // driver waits on driver done semaphore
        // TODO: save driver driverDone semaphore
        // SEM_SIGNAL(driverDone);
//        for (int i = 0; i < 3; ++i) {
//            // TODO: create rideDone semaphore for passengers
//            // TODO: save passenger rideDone[] semaphore
//            // SEM_SIGNAL(rideDone[i]);
//        }
    }
    return 0;
}

int visitorTask(int argc, char* argv[]) {
    // get in line to buy a ticket
    // get a ticket
    // only 1 visitor at a time requests a ticket
    // getTicketMutex is initialized with 12 tickets
//    SEM_WAIT(getTicketMutex);		SWAP;
//    {
//        // signal need ticket (produce, put hand up)
//        SEM_SIGNAL(needTicket);		SWAP;
//
// NOTE: need to protect this with the needDriverMutex

//        // wakeup a driver (produce)
//        SEM_SIGNAL(wakeupDriver);	SWAP;
//
//        // wait for driver to obtain ticket (consume)
//        SEM_WAIT(ticketReady);		SWAP;
//
//        // put hand down (consume, driver awake, ticket ready)
//        SEM_WAIT(needTicket);		SWAP;
//
//        // buy ticket (produce, signal driver ticket taken)
//        SEM_SIGNAL(buyTicket);		SWAP;
//    }
// // done (produce)
//    SEM_SIGNAL(getTicketMutex);		SWAP;
    // visit the museum
    // take a ride around the park
    // visit the gift shop
    // leave the park
    return 0;
}

int driverTask(int argc, char* argv[]) {
    int driverId = atoi(argv[0]);
    // each driver has a mutex (is available and asleep by default)
    // loop;
    // when signaled, check to see if the next thing needed is a driver or a ticket-seller
    // after performing the duties, go back to sleep
    char buf[32];
    sprintf(buf, "driver[%d]-Done", driverId);
    driverDone = createSemaphore(buf, BINARY, 1);
    return 0;
}

// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char *argv[]) {
    printf("\nDelta Clock");
    int i;
    for (i = 0; i < deltaClockCount; i++) {
        printf("\n%4d%4d  %-20s", i, deltaClock[i].time, deltaClock[i].sem->name);
    }
    return 0;
} // end CL3_dc

// insert into delta clock
// returns the index of the insertion
int insertIntoDeltaClock(int time, Semaphore *semaphore) {
    printf("\nInserting new item into delta clock: %d\t\t%s", time, semaphore->name);
    printDeltaClock();
    // if the delta clock is empty, then insert the item at the front and increment the clock's size
    if (deltaClockCount == 0) {
        deltaClock = (DeltaClock *) malloc(MAX_TASKS * sizeof(DeltaClock));
        for (int i = 0; i < MAX_TASKS; ++i) {
            deltaClock[i].time = 0;
            deltaClock[i].sem = NULL;
        }
        DeltaClock x = {time, semaphore};
        deltaClock[0] = x;
        deltaClockCount = 1;

        return 0;
    }

    // otherwise, iterate through the clock and find where the item belongs
    for (int i = 0; i <= deltaClockCount; ++i) {
        if (time < deltaClock[i].time || i == deltaClockCount) {
            for (int j = deltaClockCount; j > i; j--) {
                deltaClock[j] = deltaClock[j - 1];
                if (j == i + 1) {
                    deltaClock[j].time -= time;
                }
            }
            deltaClock[i].time = time;
            deltaClock[i].sem = semaphore;
            deltaClockCount += 1;
            break;
        } else {
            time -= deltaClock[i].time;
        }
    }
    return 0;
}

int popDeltaClock() {
    deltaClockCount -= 1;
    for (int i = 0; i < deltaClockCount; ++i) {
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
void printDeltaClock(void) {
    int i;
    for (i = 0; i < deltaClockCount; i++) {
        printf("\n%4d%4d  %-20s", i, deltaClock[i].time, deltaClock[i].sem->name);
    }
    return;
}


// ***********************************************************************
// test delta clock
int P3_tdc(int argc, char *argv[]) {
    createTask("DC Test",            // task name
               dcMonitorTask,        // task
               10,                    // task priority
               argc,                    // task arguments
               argv);

    timeTaskID = createTask("Time",        // task name
                            timeTask,    // task
                            10,            // task priority
                            argc,            // task arguments
                            argv);
    return 0;
} // end P3_tdc

// ***********************************************************************
// monitor the delta clock task
int dcMonitorTask(int argc, char *argv[]) {
    event = (Semaphore **) malloc(100 * sizeof(Semaphore *));
    int i, flg;
    char buf[32];
    // create some test times for event[0-9]
    int ttime[10] = {
            90, 300, 50, 170, 340, 300, 50, 300, 40, 110};

    for (i = 0; i < 10; i++) {
        sprintf(buf, "event[%d]", i);
        event[i] = createSemaphore(buf, BINARY, 0);
        insertIntoDeltaClock(ttime[i], event[i]);
    }
    printDeltaClock();

    dcChange = createSemaphore("DCChange", BINARY, 0);
    while (deltaClockCount > 0) {
        SEM_WAIT(dcChange)
        flg = 0;
        for (i = 0; i < 10; i++) {
            if (event[i]->state == 1) {
                printf("\n  event[%d] signaled", i);
                event[i]->state = 0;
                flg = 1;
            }
        }
        if (flg) printDeltaClock();
    }
    printf("\nNo more events in Delta Clock");

    // kill dcMonitorTask
    tcb[timeTaskID].state = S_EXIT;
    return 0;
} // end dcMonitorTask


extern Semaphore *tics1sec;

// ********************************************************************************************
// display time every tics1sec
int timeTask(int argc, char *argv[]) {
    char svtime[64];                        // ascii current time
    while (1) {
        SEM_WAIT(tics1sec)
        printf("\nTime = %s", myTime(svtime));
    }
    return 0;
} // end timeTask


