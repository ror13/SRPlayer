//
// Created by ror131 on 3/1/16.
//

#include "utils.h"
#include <media/stagefright/MediaBufferGroup.h>
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

void clearCMessage(MessageType type, void* data){
    if(data == NULL){
        return;
    }
    switch(type){
        case MessageType::AUDIO_PKT:
        case MessageType::VIDEO_PKT:{
            AVPacket* packet =(AVPacket*) data;
            av_free_packet(packet);
            delete packet;
            break;
        }
        case MessageType::AUDIO_CODEC_CONFIG:
        case MessageType::VIDEO_CODEC_CONFIG:{
            AVCodecContext* context = (AVCodecContext*) data;
            avcodec_free_context(&context);
            break;
        }
        case MessageType::AUDIO_RENDER_CONFIG:
        case MessageType::VIDEO_RENDER_CONFIG:{
            delete data;
            break;
        }
        case MessageType::VIDEO_FRAME:{
            CVideoFrame* vf = (CVideoFrame*) data;
            android::MediaBuffer* mediaBuffer = (android::MediaBuffer*) vf->data;
            mediaBuffer->release();
            delete data;
            break;
        }
        case MessageType::AUDIO_FRAME:{
            CAudioFrame* audioFrame = (CAudioFrame*) data;
            if(audioFrame && audioFrame->data){
                delete audioFrame->data;
            }
            delete data;
            break;
        }
        case MessageType::FILE_EOF:
            break;
    }
}

void clearCMessage(CMessage* msg){
    clearCMessage(msg->type, msg->data);
}