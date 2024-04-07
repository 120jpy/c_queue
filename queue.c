
#include <string.h>
#include "queue.h"

#ifndef __USE_MISC
#define __USE_MISC
#endif

#include <endian.h>

uint32_t ulNextIndex(uint32_t ulIndex,uint32_t ulSize);
bool bQueueIsEmpty(Queue_t* pxQueue);
void vQueueMutexInit(QueueMutex_t* pxQueueMutex_t);
void vQueueLock(Queue_t* pxQueue);
void vQueueUnlock(Queue_t* pxQueue);

void vQueueInit(Queue_t* pxQueue){
    pxQueue->xSetting.ucOFFlag = 0;
    pxQueue->xSetting.ulReadIndex = 0;
    pxQueue->xSetting.ulWriteIndex = 0;
    memset(pxQueue->aucData,0,eQueueLength);
    vQueueMutexInit(&pxQueue->xSetting.xMutex);
}

bool bQueuePush(Queue_t* pxQueue,uint8_t* pucData,uint32_t ulSize){
    uint32_t tmp = 0;
    uint32_t ulNext = 0;
    
    if(pxQueue == NULL || pucData == NULL){
        return false;
    }
    
    ulNext = ulNextIndex(pxQueue->xSetting.ulWriteIndex,ulSize);
    vQueueLock(pxQueue);

    if(ulNext < pxQueue->xSetting.ulWriteIndex){
        pxQueue->xSetting.ucOFFlag = 1;
    }
    if((pxQueue->xSetting.ucOFFlag == 1) && (ulNext > pxQueue->xSetting.ulReadIndex)){
        vQueueUnlock(pxQueue);
        return false;
    }

    *(uint32_t*)&pxQueue->aucData[pxQueue->xSetting.ulWriteIndex] = ulSize;
    pxQueue->xSetting.ulWriteIndex = (pxQueue->xSetting.ulWriteIndex + eQueueBlockSize) & eQueueLengthMask;

    tmp = (ulSize >> 2);

    if(((uint32_t)pucData & 0b11) == 0x00){
        for(uint32_t i = 0;i < tmp;i++){
            *(uint32_t*)&pxQueue->aucData[pxQueue->xSetting.ulWriteIndex] = ((uint32_t*)pucData)[i];
            pxQueue->xSetting.ulWriteIndex = (pxQueue->xSetting.ulWriteIndex + eQueueBlockSize) & eQueueLengthMask;
        }
    }else{
        for(uint32_t i = 0;i < tmp;i++){
            #if LITTLE_ENDIAN
            *(uint32_t*)&pxQueue->aucData[pxQueue->xSetting.ulWriteIndex] = (uint32_t)((pucData[(i << 2) + 3] << 24 )
                                         |(pucData[(i << 2) + 2] << 16 ) |(pucData[(i << 2) + 1] << 8 ) |(pucData[(i << 2)]));
            #else
            *(uint32_t*)&pxQueue->aucData[pxQueue->xSetting.ulWriteIndex] = (uint32_t)((pucData[(i << 2)] << 24 ) 
                                        |(pucData[(i << 2) + 1] << 16 ) |(pucData[(i << 2) + 2] << 8 ) |(pucData[(i << 2) + 3]));
            #endif
            pxQueue->xSetting.ulWriteIndex = (pxQueue->xSetting.ulWriteIndex + eQueueBlockSize) & eQueueLengthMask;
        }
    }

    tmp <<= 2;
    for(uint32_t i = 0;i < (ulSize & 0b11);i++){
        pxQueue->aucData[pxQueue->xSetting.ulWriteIndex + i] = pucData[tmp + i];
    }
    if(ulSize & 0b11){
        pxQueue->xSetting.ulWriteIndex = (pxQueue->xSetting.ulWriteIndex + eQueueBlockSize) & eQueueLengthMask;
    }
    vQueueUnlock(pxQueue);
    return true;

}

bool bQueuePop(Queue_t* pxQueue,uint8_t* pucData,uint32_t* pulSize){
    uint32_t ulSize = *(uint32_t*)&pxQueue->aucData[pxQueue->xSetting.ulReadIndex];
    uint32_t ulNext = ulNextIndex(pxQueue->xSetting.ulReadIndex,ulSize);
    uint32_t tmp = 0;

    if(pxQueue == NULL || pucData == NULL || pulSize == NULL){
        return false;
    }
    
    if(*pulSize < ulSize){
        return false;
    }
    
    vQueueLock(pxQueue);
    if(ulNext < pxQueue->xSetting.ulReadIndex){
        pxQueue->xSetting.ucOFFlag = 0;
    }
    if(bQueueIsEmpty(pxQueue) == true){
        vQueueUnlock(pxQueue);
        return false;
    }
    
    *pulSize = ulSize;
    pxQueue->xSetting.ulReadIndex = (pxQueue->xSetting.ulReadIndex + eQueueBlockSize) & eQueueLengthMask;
    tmp = (ulSize >> 2);

    if(((uint32_t)pucData & 0b11) == 0x00){
        for(uint32_t i = 0;i < tmp;i++){
            ((uint32_t*)pucData)[i] = *(uint32_t*)&pxQueue->aucData[pxQueue->xSetting.ulReadIndex];
            pxQueue->xSetting.ulReadIndex = (pxQueue->xSetting.ulReadIndex + eQueueBlockSize) & eQueueLengthMask;
        }

        tmp *= eQueueBlockSize;
    }else{
        tmp *= eQueueBlockSize;
        for(uint32_t i = 0;i < tmp;i+=eQueueBlockSize){
            pucData[i] = (uint8_t)(pxQueue->aucData[pxQueue->xSetting.ulReadIndex]);
            pucData[i+ 1] = (uint8_t)(pxQueue->aucData[pxQueue->xSetting.ulReadIndex + 1]);
            pucData[i+ 2] = (uint8_t)(pxQueue->aucData[pxQueue->xSetting.ulReadIndex + 2]);
            pucData[i+ 3] = (uint8_t)(pxQueue->aucData[pxQueue->xSetting.ulReadIndex + 3]);
            pxQueue->xSetting.ulReadIndex = (pxQueue->xSetting.ulReadIndex + eQueueBlockSize) & eQueueLengthMask;
        }
    }
    
    for(uint32_t i = 0;i < (ulSize & 0b11);i++){
        pucData[tmp + i] = pxQueue->aucData[pxQueue->xSetting.ulReadIndex + i];

    }
    if(ulSize & 0b11){
        pxQueue->xSetting.ulReadIndex = (pxQueue->xSetting.ulReadIndex + eQueueBlockSize) & eQueueLengthMask;
    }
    vQueueUnlock(pxQueue);
    return true;
}

bool bQueueIsEmpty(Queue_t* pxQueue){
    if((pxQueue->xSetting.ucOFFlag == 0) && (pxQueue->xSetting.ulWriteIndex == pxQueue->xSetting.ulReadIndex)){
        return true;
    }
    return false;
}


uint32_t ulNextIndex(uint32_t ulIndex,uint32_t ulSize){
    uint32_t ulNext = 0;
    if((ulSize & 0b11) == 0x00){
        ulNext = ulSize;
    }else{
        ulNext = (ulSize & ~(0b11)) + eQueueBlockSize;
    }
    ulNext = (ulIndex + ulSize) & eQueueLengthMask;
    return ulNext;
}

void vQueueMutexInit(QueueMutex_t* pxQueueMutex_t){
    // Mutex or semaphore Init
}

void vQueueLock(Queue_t* pxQueue){
    // Code to lock the Mutex or semaphore to be use
}

void vQueueUnlock(Queue_t* pxQueue){
    // Code to unlock the Mutex or semaphore to use
}
