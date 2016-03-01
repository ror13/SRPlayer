//
// Created by ror131 on 3/1/16.
//

#include "utils.h"
static unsigned long FPS_ThisTime = 0;
static unsigned long FPS_LastTime = 0;
static unsigned long FPS_Count = 0;
static int FPS_TimeCount = 0;




unsigned long GFps_GetCurFps()
{
    if (FPS_TimeCount == 0) {
        FPS_LastTime = getTime()/1000;
    }
    if (++FPS_TimeCount >= 30) {
        if (FPS_LastTime - FPS_ThisTime != 0) {
            FPS_Count = 30000 / (FPS_LastTime - FPS_ThisTime);
        }
        FPS_TimeCount = 0;
        FPS_ThisTime = FPS_LastTime;
    }
    return FPS_Count;
}

int64_t getTime(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}