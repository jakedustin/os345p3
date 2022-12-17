// ***********************************************************************
// project 3 variables

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <execinfo.h>
#include "os345park.h"
#include "os345.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
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

// some constants that are helpful
const int MAX_TIME_WAITING_OUTSIDE = 100;       // 100 milliseconds == 10 seconds
const int MAX_TIME_WAITING_INSIDE = 30;         // 30 milliseconds == 3 seconds

// ******************************************************************* //
//                         RESOURCE SEMAPHORES                         //
// ******************************************************************* //
Semaphore *tickets;

// ******************************************************************* //
//                          MUTEX SEMAPHORES                           //
// ******************************************************************* //
Semaphore *getTicketMutex;
Semaphore *mailboxMutex;
Semaphore *carIdMutex;

//driverDoneMutex protects the driverDoneSemaphore that the driver's semaphore is passed through
Semaphore *driverDoneMutex;

// ******************************************************************* //
//                          SIGNAL SEMAPHORES                          //
// ******************************************************************* //
Semaphore *wakeupDriver;
Semaphore *seatTaken;

// mailbox flag signals when there is something inside it
Semaphore *mailboxReady;

// mailbox flag signals when the semaphore has been taken from the mailbox
Semaphore *mailAcquired;

// needDriver signals when a car is ready for a driver
Semaphore *needDriver;

// driverDoneSemaphore functions like a mailbox for the driver and the car to communicate
Semaphore *driverDoneSemaphore;

// driver needs to signal when the driver's semaphore has been placed in the driverDoneSemaphore
Semaphore *driverReady;

// car needs to signal when the car has saved the driver's semeaphore
Semaphore *carReady;

Semaphore *getPassenger;
Semaphore *passengerSeated;
Semaphore *driverInCar;
Semaphore *goodToGo;
Semaphore *needTicket;
Semaphore *takeTicket;
Semaphore *ticketReady;
Semaphore *buyTicket;
Semaphore *moveCars;

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

extern void printStackTrace();

int getRandomAmountOfInsideTime();

int getRandomAmountOfOutsideTime();


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
//    printf("\nStart Jurassic Park...");
//    fflush(stdout);
//    SWAP;
//    printf("\nSetting park variables");
//    fflush(stdout);
    // myPark.numInCarLine = myPark.numInPark = 4;
    // SWAP;

    // initialize semaphores
    tickets = createSemaphore("tickets", COUNTING, MAX_TICKETS);        SWAP;
    needTicket = createSemaphore("needTicket", BINARY, 0);              SWAP;
    takeTicket = createSemaphore("takeTicket", BINARY, 0);              SWAP;
    ticketReady = createSemaphore("ticketReady", BINARY, 0);            SWAP;
    getTicketMutex = createSemaphore("getTicketMutex", BINARY, 1);      SWAP;

    mailboxMutex = createSemaphore("mailboxMutex", BINARY, 1);          SWAP;
    mailboxReady = createSemaphore("mailboxReady", BINARY, 0);          SWAP;
    mailAcquired = createSemaphore("mailAcquired", BINARY, 0);          SWAP;
    getPassenger = createSemaphore("getPassenger", BINARY, 0);          SWAP;
    passengerSeated = createSemaphore("passengerSeated", BINARY, 0);    SWAP;
    seatTaken = createSemaphore("seatTaken", BINARY, 0);                SWAP;
    wakeupDriver = createSemaphore("wakeupDriver", BINARY, 0);          SWAP;
    driverReady = createSemaphore("driverReady", BINARY, 0);            SWAP;
    carReady = createSemaphore("carReady", BINARY, 0);                  SWAP;
    carIdMutex = createSemaphore("carIdMutex", BINARY, 1);              SWAP;

    needDriver = createSemaphore("needDriver", BINARY, 0);              SWAP;
    driverDoneSemaphore = createSemaphore("driverDone", BINARY, 0);     SWAP;
    driverDoneMutex = createSemaphore("driverDoneMutex", BINARY, 1);    SWAP;

    // ?? create car, driver, and visitor tasks here
    for (int i = 0; i < NUM_CARS; ++i)
    {
        char *intStr = (char *)malloc(8 * sizeof(char));
        // put values in strings
        sprintf(buf, "car[%d]", i);                             SWAP;
        sprintf(intStr, "%d", i);                               SWAP;

        // move car Id into new argvs
        strcpy(newArgv[0], intStr);                             SWAP;

        // create car task
        createTask(buf, carTask, MED_PRIORITY, 1, newArgv);     SWAP;
    }

    int numVisitors = 45;
    for (int i = 0; i < numVisitors; i++) {
        char *intStr = (char *)malloc(8 * sizeof(char));    SWAP;
        // put values in strings
        sprintf(buf, "visitor[%d]", i);                          SWAP;
        sprintf(intStr, "%d", i);                                SWAP;

        // move visitor Id into new args
        strcpy(newArgv[0], intStr);                              SWAP;

        // create visitor task
        createTask(buf, visitorTask, MED_PRIORITY, 1, newArgv);  SWAP;
    }

    for (int i = 0; i < NUM_DRIVERS; i++) {
        char *intStr = (char *)malloc(8 * sizeof(char));    SWAP;

        sprintf(buf, "driver[%d]", i);                           SWAP;
        sprintf(intStr, "%d", i);                                SWAP;

        strcpy(newArgv[0], intStr);                              SWAP;

        createTask(buf, driverTask, MED_PRIORITY, 1, newArgv);   SWAP;
    }

    return 0;
} // end project3

/************************************************************************/
/******************************* CAR TASK *******************************/
/************************************************************************/
int carTask(int argc, char *argv[])
{
    // get carId from argvs
    int carId = atoi(argv[0]);                                  SWAP;

    // create array of rideDone semaphores to signal driver and passengers when the ride is finished
    Semaphore *rideDone[4];                                     SWAP;

    while (1) {
        // loop through each seat in the car
        for (int i = 0; i < 3; ++i) {
            SEM_WAIT(fillSeat[carId]);          SWAP;
            // signal that the car is ready for a passenger
            SEM_SIGNAL(getPassenger);           SWAP;
            // get passenger information from the mailbox
            SEM_WAIT(mailboxReady);             SWAP;
                rideDone[i] = mailbox;          SWAP;
            SEM_SIGNAL(mailAcquired);           SWAP;
            SEM_SIGNAL(passengerSeated);        SWAP;

            // on the final seat, get a driver and mark the car as ready to go
            if (i == 2) {
                // NOTES: signal that a driver is needed; the driver is waiting on this semaphore
                SEM_SIGNAL(needDriver);             SWAP;
                // NOTES: signal that the driver is needed
                SEM_SIGNAL(wakeupDriver);           SWAP;
                // NOTES: save the driver's rideDone semaphore
                SEM_WAIT(driverReady);              SWAP;
                rideDone[3] = driverDoneSemaphore;  SWAP;
                SEM_WAIT(carIdMutex);               SWAP;
                    carIdHolder = carId;            SWAP;
                SEM_SIGNAL(carIdMutex);             SWAP;
                SEM_SIGNAL(carReady);               SWAP;
            }
            SEM_SIGNAL(seatFilled[carId]);      SWAP;
        }

        SEM_WAIT(rideOver[carId]);              SWAP;
        printf("\nrideOver[%d] was signaled", carId);
        for (int i = 0; i < 4; ++i) {
            SEM_SIGNAL(rideDone[i]);            SWAP;
        }
    }

    return 0;
}


// ************************************************************************ //
// ******************************* CAR TASK ******************************* //
// ************************************************************************ //
int visitorTask(int argc, char *argv[]) {
    int visitorId = atoi(argv[0]);                                      SWAP;
    char *name = (char*)malloc(10 * sizeof(char));                 SWAP;
    sprintf(name, "visitor[%d]", visitorId);                            SWAP;

    // NOTES: create visitor semaphore: used to communicate between the car and visitor and other park functions.
    Semaphore *visitorSemaphore = createSemaphore(name, BINARY, 0); SWAP;

    // NOTES: wait to queue up outside of the park
    SEM_WAIT(parkMutex);        SWAP;
        insertIntoDeltaClock(getRandomAmountOfOutsideTime(), visitorSemaphore);                SWAP;
    SEM_SIGNAL(parkMutex);      SWAP;
    SEM_WAIT(visitorSemaphore);                                                SWAP;

    // NOTES: update num outside park after random amount of time
    SEM_WAIT(parkMutex);    SWAP;
        myPark.numOutsidePark++;
    SEM_SIGNAL(parkMutex);  SWAP;

    /********/
    SEM_WAIT(parkMutex);    SWAP;
        insertIntoDeltaClock(getRandomAmountOfInsideTime(), visitorSemaphore); SWAP;
    SEM_SIGNAL(parkMutex);  SWAP;
    SEM_WAIT(visitorSemaphore); SWAP;
    /********/

    // NOTES: update number of visitors in and outside the park
    // NOTES: wait for park mutex to become available
    SEM_WAIT(parkMutex);                                                   SWAP;
        myPark.numOutsidePark--;                                           SWAP;
        myPark.numInPark++;                                                SWAP;
        myPark.numInTicketLine++;                                          SWAP;
        // NOTES: insert visitor semaphore into the delta clock to wait to request a ticket
        insertIntoDeltaClock(getRandomAmountOfInsideTime(), visitorSemaphore);            SWAP;
    // NOTES: release park mutex
    SEM_SIGNAL(parkMutex);                                                 SWAP;
    SEM_WAIT(visitorSemaphore);                                            SWAP;


    /****** ACQUIRE TICKET ******/

    // NOTES: signal for the driver to get the ticket
    SEM_WAIT(getTicketMutex);           SWAP;
        SEM_SIGNAL(needTicket);         SWAP;
        SEM_SIGNAL(wakeupDriver);       SWAP;
        SEM_WAIT(ticketReady);          SWAP;
        SEM_SIGNAL(takeTicket);         SWAP;
    SEM_SIGNAL(getTicketMutex);         SWAP;


    // NOTES: leave the ticket line, decrement the number of available tickets, and get in the museum line
    SEM_WAIT(parkMutex);                                                    SWAP;
        myPark.numInTicketLine--;                                           SWAP;
        myPark.numTicketsAvailable--;                                       SWAP;
        myPark.numInMuseumLine++;                                           SWAP;
        // NOTES: wait a random amount of time in line before entering the museum
        insertIntoDeltaClock(getRandomAmountOfInsideTime(), visitorSemaphore);            SWAP;
    SEM_SIGNAL(parkMutex);                                                  SWAP;
    SEM_WAIT(visitorSemaphore);                                            SWAP;


    // NOTES: enter the museum
    SEM_WAIT(parkMutex);                                                                  SWAP;
        myPark.numInMuseumLine--;                                                         SWAP;
        myPark.numInMuseum++;                                                             SWAP;
        insertIntoDeltaClock(getRandomAmountOfInsideTime(), visitorSemaphore);            SWAP;
    SEM_SIGNAL(parkMutex);                                                                SWAP;
    SEM_WAIT(visitorSemaphore);                                                           SWAP;

    // NOTES: leave museum and enter the car line
    SEM_WAIT(parkMutex);                                                    SWAP;
        myPark.numInMuseum--;                                               SWAP;
        myPark.numInCarLine++;                                              SWAP;
        // NOTES: wait random amount of time in car line
        insertIntoDeltaClock(getRandomAmountOfInsideTime(), visitorSemaphore);  SWAP;
    SEM_SIGNAL(parkMutex);                                                  SWAP;

    // NOTES: wait for a car to signal that it is ready for a passenger
    SEM_WAIT(getPassenger);             SWAP;

    // NOTES: wait for the mailbox to be available
    SEM_WAIT(mailboxMutex);             SWAP;
        // NOTES: save visitor semaphore in the mailbox for access by the car
        mailbox = visitorSemaphore;     SWAP;
        // NOTES: signal to the car that the semaphore has been placed in the mailbox
        SEM_SIGNAL(mailboxReady);       SWAP;
        // NOTES: wait for the car to signal that it has received the mail from the mailbox
        SEM_WAIT(mailAcquired);         SWAP;
    // NOTES: release the mailbox mutex
    SEM_SIGNAL(mailboxMutex);           SWAP;

    // NOTES: wait for the car to signal that the passenger has been seated and rideDone semaphore set
    SEM_WAIT(passengerSeated);
    SEM_WAIT(parkMutex);                                                    SWAP;
        myPark.numInCarLine--;                                              SWAP;
        myPark.numInCars++;                                                 SWAP;
        SEM_SIGNAL(tickets);                                                SWAP;
        myPark.numTicketsAvailable++;                                       SWAP;
    SEM_SIGNAL(parkMutex);                                                  SWAP;

    // NOTES: wait for the car to signal that the ride is over
    SEM_WAIT(visitorSemaphore);                                             SWAP;

    // NOTES: leave the car, get in the gift line
    SEM_WAIT(parkMutex);                                                    SWAP;
        myPark.numInCars--;                                                 SWAP;
        myPark.numInGiftLine++;                                             SWAP;
        // NOTES: wait a random amount of time in the gift line before entering the gift shop
        insertIntoDeltaClock(getRandomAmountOfInsideTime(), visitorSemaphore);    SWAP;
    SEM_SIGNAL(parkMutex);                                                  SWAP;
    SEM_WAIT(visitorSemaphore);                                             SWAP;

    // NOTES: leave gift line, enter gift shop
    SEM_WAIT(parkMutex);                                                    SWAP;
        myPark.numInGiftLine--;                                             SWAP;
        myPark.numInGiftShop++;                                             SWAP;
        // NOTES: hang out inside the gift shop for a random amount of time
        insertIntoDeltaClock(getRandomAmountOfInsideTime(), visitorSemaphore); SWAP;
    SEM_SIGNAL(parkMutex);                                                  SWAP;
    // NOTES: wait for the random amount of time to be over
    SEM_WAIT(visitorSemaphore);                                             SWAP;

    // NOTES: leave gift shop and exit the park
    SEM_WAIT(parkMutex);                                                    SWAP;
        myPark.numInGiftShop--;                                             SWAP;
        myPark.numInPark--;                                                 SWAP;
        myPark.numExitedPark++;                                             SWAP;
    SEM_SIGNAL(parkMutex);                                                  SWAP;

    // NOTES: Kill visitor task
    return 0;
}

int driverTask(int argc, char *argv[]) {
    // NOTES: driverDone is used to track the driver across various tasks
    Semaphore *driverDone;

    char buf[32];
    int driverId = atoi(argv[0]);                       SWAP;
    int carId;
    sprintf(buf, "driverDone[%d]", driverId);           SWAP;
    driverDone = createSemaphore(buf, BINARY, 0);   SWAP;

    while (1) {
        // NOTES: wait for another task to request a driver
        SEM_WAIT(wakeupDriver);                         SWAP;
        if (semTryLock(needDriver)) {
            SEM_WAIT(driverDoneMutex);                  SWAP;
                driverDoneSemaphore = driverDone;       SWAP;
            // NOTES: driverReady is used to signal that the driver's semaphore has been placed in the box
                SEM_SIGNAL(driverReady);                SWAP;
            SEM_SIGNAL(driverDoneMutex);                SWAP;
            // NOTES: carReady is used by the car to acknowledge that the driver's semaphore has been received
            SEM_WAIT(carReady);                         SWAP;
            carId = carIdHolder;                        SWAP;
            SEM_WAIT(parkMutex);                        SWAP;
                myPark.drivers[driverId] = carId + 1;   SWAP;
            SEM_SIGNAL(parkMutex);                      SWAP;
            // NOTES: wait for the car to signal that the ride is over
            SEM_WAIT(driverDone);                       SWAP;
            SEM_WAIT(parkMutex);                        SWAP;
                myPark.drivers[driverId] = 0;           SWAP;
            SEM_SIGNAL(parkMutex);                      SWAP;
        } else if (semTryLock(needTicket)) {
            printf("\nDriver[%d]: selling tickets", driverId);
            SEM_WAIT(parkMutex);                        SWAP;
                myPark.drivers[driverId] = -1;          SWAP;
            SEM_SIGNAL(parkMutex);                      SWAP;

            printf("Driver[%d]: successfully updated myPark variable", driverId);
            SEM_WAIT(tickets);                          SWAP;
            SEM_SIGNAL(ticketReady);                    SWAP;
            SEM_WAIT(takeTicket);                       SWAP;

            SEM_WAIT(parkMutex);                        SWAP;
                myPark.drivers[driverId] = 0;           SWAP;
            SEM_SIGNAL(parkMutex);                      SWAP;
        } else {
            printf("\nDriver[%d]: Breaking for some reason!", driverId);
            break;
        }
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
    if (!dcChange) {
        dcChange = createSemaphore("DCChange", BINARY, 0);
    }
    // printf("\nInserting new item into delta clock: %d\t\t%s", time, semaphore->name);
    // printDeltaClock();      SWAP;
    // if the delta clock is empty, then insert the item at the front and increment the clock's size
    if (deltaClockCount == 0)
    {
        deltaClock = (DeltaClock *)malloc(MAX_TASKS * sizeof(DeltaClock));  SWAP;
        for (int i = 0; i < MAX_TASKS; ++i)
        {
            deltaClock[i].time = 0;     SWAP;
            deltaClock[i].sem = NULL;   SWAP;
        }
        DeltaClock x = {time, semaphore};   SWAP;
        deltaClock[0] = x;  SWAP;
        deltaClockCount = 1;    SWAP;

        return 0;
    }

    // otherwise, iterate through the clock and find where the item belongs
    for (int i = 0; i <= deltaClockCount; ++i)
    {
        if (time < deltaClock[i].time || i == deltaClockCount)
        {
            for (int j = deltaClockCount; j > i; j--)
            {
                deltaClock[j] = deltaClock[j - 1];  SWAP;
                if (j == i + 1)
                {
                    deltaClock[j].time -= time; SWAP;
                }
            }
            deltaClock[i].time = time;  SWAP;
            deltaClock[i].sem = semaphore;  SWAP;
            deltaClockCount += 1;   SWAP;
            break;
        }
        else
        {
            time -= deltaClock[i].time; SWAP;
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

int getRandomAmountOfInsideTime() {
    return rand() % (MAX_TIME_WAITING_INSIDE + 1);
}

int getRandomAmountOfOutsideTime() {
    return rand() % (MAX_TIME_WAITING_OUTSIDE + 1);
}

void printStackTrace() {
    void* callstack[128];
    int i, frames = backtrace(callstack, 128);
    char** backtraceSymbols = backtrace_symbols(callstack, frames);
    for (i = 0; i < frames; ++i) {
        printf("\n%s", backtraceSymbols[i]);
    }
    printf("\n");
    free(backtraceSymbols);
}
#pragma clang diagnostic pop