
#include <string.h>
#include "queue.h"

#ifndef __USE_MISC
#define __USE_MISC
#endif

#include <endian.h>

uint32_t ulNextIndex(uint32_t ulIndex,uint32_t ulSize);
bool bQueueIsEmpty(Queue_t* pxQueue);
void vQueueLock(Queue_t* pxQueue);
void vQueueUnlock(Queue_t* pxQueue);

void vQueueInit(Queue_t* pxQueue){
    pxQueue->xSetting.ucOFFlag = 0;
    pxQueue->xSetting.ulReadIndex = 0;
    pxQueue->xSetting.ulWriteIndex = 0;
    memset(pxQueue->aucData,0,eQueueLength);
}

bool bQueuePush(Queue_t* pxQueue,uint8_t* pucData,uint32_t ulSize){
    uint32_t tmp = 0;
    uint32_t ulNext = ulNextIndex(pxQueue->xSetting.ulWriteIndex,ulSize);
    
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
            *(uint32_t*)&pxQueue->aucData[pxQueue->xSetting.ulWriteIndex] = (uint32_t)((pucData[i] << 24 ) |(pucData[i + 1] << 16 ) |(pucData[i + 2] << 8 ) |(pucData[i + 3]));
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
    }else{
        for(uint32_t i = 0;i < tmp;i++){
            #if LITTLE_ENDIAN
            for(uint32_t j = 0;j < 4;j++){
                pucData[(i << 2)+ j] = (uint8_t)(pxQueue->aucData[pxQueue->xSetting.ulReadIndex + j]);
            }
            #else
            for(uint32_t j = 0;j < 4;j++){
                pucData[(i << 2)+ j] = (uint8_t)(pxQueue->aucData[pxQueue->xSetting.ulReadIndex + (4 -j)]);
            }
            #endif
            pxQueue->xSetting.ulReadIndex = (pxQueue->xSetting.ulReadIndex + eQueueBlockSize) & eQueueLengthMask;
        }
    }

    tmp *= eQueueBlockSize;
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
        ulNext =( (ulSize >> 2) << 2) + eQueueBlockSize;
    }
    ulNext = (ulIndex + ulSize) & eQueueLengthMask;
    return ulNext;
}

void vQueueLock(Queue_t* pxQueue){
    // Code to lock the Mutex or semaphore to be use
}

void vQueueUnlock(Queue_t* pxQueue){
    // Code to unlock the Mutex or semaphore to use
}

