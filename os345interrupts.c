// os345interrupts.c - pollInterrupts	06/21/2020
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
#include "os345config.h"
#include "os345signals.h"

// **********************************************************************
//	local prototypes
//
void pollInterrupts(void);

static void keyboard_isr(void);

static void timer_isr(void);

// **********************************************************************
// **********************************************************************
// global semaphores

extern Semaphore *keyboard;                // keyboard semaphore
extern Semaphore *charReady;                // character has been entered
extern Semaphore *inBufferReady;            // input buffer ready semaphore

extern Semaphore *tics1sec;                // 1 second semaphore
extern Semaphore *tics10thsec;                // 1/10 second semaphore
extern Semaphore *tics10sec;
extern Semaphore *dcChange;

extern DeltaClock *deltaClock;
extern int deltaClockCount;
extern int popDeltaClock();

extern char inChar;                // last entered character
extern int charFlag;                // 0 => buffered input
extern int inBufIndx;                // input pointer into input buffer
extern char inBuffer[INBUF_SIZE + 1];    // character input buffer

extern time_t oldTime1;                    // old 1sec time
extern time_t oldTime10;
extern clock_t myClkTime;
extern clock_t myOldClkTime;

extern int pollClock;                // current clock()
extern int lastPollClock;            // last pollClock

extern int superMode;                        // system mode
extern long swapCount;


// **********************************************************************
// **********************************************************************
// simulate asynchronous interrupts by polling events during idle loop
//
void pollInterrupts(void) {
    // check for task monopoly
    pollClock = clock();
    assert("Timeout" && ((pollClock - lastPollClock) < MAX_CYCLES));
    lastPollClock = pollClock;

    // check for keyboard interrupt
    if ((inChar = GET_CHAR) > 0) {
        keyboard_isr();
    }

    // timer interrupt
    timer_isr();

    return;
} // end pollInterrupts


// **********************************************************************
// keyboard interrupt service routine
//
static void keyboard_isr() {
    // assert system mode
    assert("keyboard_isr Error" && superMode);

    semSignal(charReady);                    // SIGNAL(charReady) (No Swap)
    if (charFlag == 0) {
        switch (inChar) {
            case '\r':
            case '\n': {
                inBufIndx = 0;                // EOL, signal line ready
                semSignal(inBufferReady);     // SIGNAL(inBufferReady)
                break;
            }

            case 0x18:                        // ^x
            {
                inBufIndx = 0;
                inBuffer[0] = 0;
                sigSignal(0, mySIGINT);        // interrupt task 0
                semSignal(inBufferReady);     // SEM_SIGNAL(inBufferReady)
                break;
            }

            case 0x17:                        // ^w
            {
                printf("\ncalling sigSignal");
                sigSignal(-1, mySIGTSTP);
                break;
            }

            case 0x12:                        // ^r
            {
                sigSignal(-1, mySIGCONT);
                break;
            }

            case 0x7f:                       // backspace
            {
                if (inBufIndx > 0) {

                    printf("\b \b");
                    inBuffer[--inBufIndx] = 0;
                }
                break;
            }

            default: {
                if (inBufIndx + 1 > 255) {
                    printf("\nThat's too many letters, friendo.");
                    inBufIndx = 0;                // EOL, signal line ready
                    break;
                }
                inBuffer[inBufIndx] = inChar;
                inBuffer[++inBufIndx] = 0;
                printf("%c", inChar);        // echo character
            }
        }
    } else {
        // single character mode
        inBufIndx = 0;
        inBuffer[inBufIndx] = 0;
    }
    return;
} // end keyboard_isr


// **********************************************************************
// timer interrupt service routine
//
static void timer_isr() {
    time_t currentTime;                        // current time

    // assert system mode
    assert("timer_isr Error" && superMode);

    // capture current time
    time(&currentTime);

    // one second timer
    if ((currentTime - oldTime1) >= 1) {
        // signal 1 second
        semSignal(tics1sec);
        oldTime1 += 1;
    }

    // sample fine clock
    // if the delta clock is not empty, decrement the top item
    // if the top item's clock is now zero, signal its semaphore and pop the clock
    myClkTime = clock();
    if ((myClkTime - myOldClkTime) >= ONE_TENTH_SEC) {
        myOldClkTime = myOldClkTime + ONE_TENTH_SEC;   // update old
        if (deltaClockCount > 0) {

            if (swapCount > 6) {
            }
            deltaClock[0].time -= 1;
            if (deltaClock[0].time <= 0) {
                semSignal(deltaClock[0].sem);
                popDeltaClock();
            }
        }
        semSignal(tics10thsec);
    }

    // add other timer sampling/signaling code here for project 2
    if ((currentTime - oldTime10) >= 10) {
        SEM_SIGNAL(tics10sec);
        oldTime10 = currentTime;
    }

    return;
} // end timer_isr
