//
// Created by ror131 on 3/1/16.
//

#ifndef SYSRAZPLAYER_CVIDEO_H
#define SYSRAZPLAYER_CVIDEO_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <android/native_window.h>
}



#include "utils.h"
#include "CGlesRender.h"
#include "CDemuxer.h"

class CVideoRender : public CThread{
public:
    enum {
        GLES_WINDOW,
        NATIVE_WINDOW
    };
    CVideoRender(CQueue <CVideoFrame*>* inputQueue, ANativeWindow * nativeWindow);
    void drawFrame(void * data); // draw data on surface and swap surface
    void makeRender(); // created window
    void initializeRender(int colorspace, int frameWidth, int frameHeight); // configure curfaces
    void desroyRender(); // clear config curfaces
    void setUsingWindowType(int type){mWindowType=type;}
    void setUsingPts(bool isUsingPts){mIsUsingPts = isUsingPts;}

protected:
    static void * queueVideoRendering(void * baseObj);
    CQueue <CVideoFrame*> * mInputQueue;
    ANativeWindow * mNativeWindow;
    bool mIsUsingPts;
    int32_t mWindowType;
    CGlesRender*mRenderOpenGles;
    AVPixelFormat mColorspace;
    int32_t mFrameWidth;
    int32_t mFrameHeight;


};


class CDecoder : public CThread {
public:
    CDecoder(CDemuxer * demuxer);
    CQueue <CVideoFrame*>* getOutQueue(){return &mOutQueue;}
    void flush();
    void setFlushDemuxer(bool needFlush){mIsFlushDemuxer = needFlush;}
    void setNativeWindow(ANativeWindow* nativeWindow){mNativeWindow = nativeWindow;}
protected:
    static void * queueVideoDecoding(void * baseObj);
    CDemuxer * mDemuxer;
    ANativeWindow * mNativeWindow;
    CQueue <CVideoFrame*> mOutQueue;
    bool mIsFlushDemuxer;
};



#endif //SYSRAZPLAYER_CVIDEO_H
