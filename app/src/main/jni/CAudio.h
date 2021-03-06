//
// Created by ror131 on 3/1/16.
//

#ifndef SYSRAZPLAYER_CAUDIO_H
#define SYSRAZPLAYER_CAUDIO_H

extern "C" {
#include <libavcodec/avcodec.h>
#include "libswresample/swresample.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
}

#include "utils.h"
#include "CDemuxer.h"


class CAudioDecoder : public CThread {
public:
    CAudioDecoder(CQueue <CMessage>* inputQueue);
    void configure(AVCodecContext* codecContext);
    void clear();
    CQueue <CMessage>* getOutQueue(){return &mOutQueue;}
    void flush();
protected:
    static void * queueAudioDecoding(void * baseObj);
    CQueue <CMessage>* mInputQueue;
    CQueue <CMessage> mOutQueue;
    SwrContext* mSwrContext;
    AVCodecContext* mCodecContext;
};


class CAudioRender : public CThread{
public:
    CAudioRender(CQueue <CMessage>* inputQueue);
    ~CAudioRender();
    void configure(int32_t chanels, int32_t samplerate);
    void clear();

protected:
    static void * queueAudioRender(void * baseObj);
    static void bufferQueuePlayerCallBack (SLBufferQueueItf bufferQueue, void *baseObj);

    CQueue <CMessage>* mInputQueue;

    SLObjectItf mOutputMixObj;
    SLEngineItf mEngine;
    SLObjectItf mEngineObj;
    SLObjectItf mPlayerObject;
    SLPlayItf mPlayer;
    SLBufferQueueItf mBufferQueue;

    void * mCurrentPlayingBuff;

};

#endif //SYSRAZPLAYER_CAUDIO_H
