
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum eQueueSetting{

    eQueueBlockSize = 4,
    eQueueLength = (8 * 1024),
    eQueueLengthMask = eQueueLength - 1,
    
}eQueueSetting;

typedef uint32_t QueueMutex_t;

typedef struct QueueSetting_t{
    QueueMutex_t xMutex;
    uint32_t ulWriteIndex;
    uint32_t ulReadIndex;
    uint8_t ucOFFlag;
}QueueSetting_t;

typedef struct Queue_t{
    QueueSetting_t xSetting;
    uint8_t aucData[eQueueLength];
}Queue_t;

void vQueueInit(Queue_t* pxQueue);
bool bQueuePush(Queue_t* pxQueue,uint8_t* pucData,uint32_t ulSize);
bool bQueuePop(Queue_t* pxQueue,uint8_t* pucData,uint32_t* pulSize);
