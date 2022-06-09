#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include <time.h>

typedef int* PQueue;
typedef int TID;
typedef int Priority;

void decrementPQueue(PQueue q, size_t index);
int enQ(PQueue q, TID tid, Priority p);
int deQ(PQueue q, TID tid);
int16_t getTid(int32_t combinedKey);
int16_t getPriority(int32_t combinedKey);

int main (int argc, char* argv[]) {
    PQueue newPQueue = (int*)malloc(10 * sizeof(int));
    enQ(newPQueue, 16, 32);
    enQ(newPQueue, 32, 33);
    enQ(newPQueue, 30, 10);
    enQ(newPQueue, 8, 40);
    printf("len(newPQueue): %d", newPQueue[0]);
    for (size_t i = 1; i <= newPQueue[0]; ++i) {
        printf("\ni: %zu\t\ttid: %d\t\t\tpriority: %d", i, getTid(newPQueue[i]), getPriority(newPQueue[i]));
    }
    deQ(newPQueue, -1);
    printf("\nlen(newPQueue): %d", newPQueue[0]);
    for (size_t i = 1; i <= newPQueue[0]; ++i) {
        printf("\ni: %zu\t\ttid: %d\t\t\tpriority: %d", i, getTid(newPQueue[i]), getPriority(newPQueue[i]));
    }
    deQ(newPQueue, 30);
    printf("\nlen(newPQueue): %d", newPQueue[0]);
    for (size_t i = 1; i <= newPQueue[0]; ++i) {
        printf("\ni: %zu\t\ttid: %d\t\t\tpriority: %d", i, getTid(newPQueue[i]), getPriority(newPQueue[i]));
    }
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
