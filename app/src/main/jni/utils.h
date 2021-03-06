//
// Created by ror131 on 3/1/16.
//

#ifndef SYSRAZPLAYER_UTILS_H
#define SYSRAZPLAYER_UTILS_H
extern "C" {
#include <unistd.h>
#include <libavformat/avformat.h>
}
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <new>
#include <string>
#include <map>
#include <mutex>
#include <deque>
#include <tuple>

#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_INFO, "LOG_TAG", __VA_ARGS__)
#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, "LOG_TAG", __VA_ARGS__)
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, "LOG_TAG", __VA_ARGS__)
#define DEBUG_PRINT_LINE { LOG_ERROR(  "%s ::: %s ::: <%d>:\n", __FILE__,__FUNCTION__, __LINE__ ); }
#define kKeyTimePTS 666
#define NATIVE_WINDOW_BUFFER_COUNT 1


unsigned long GFps_GetCurFps();
int64_t getTime();

class CMutex{
public:
    CMutex(){mMx = PTHREAD_MUTEX_INITIALIZER;};
    void acquire(){pthread_mutex_lock(&this->mMx);};
    void release(){pthread_mutex_unlock(&this->mMx);};
protected:
    pthread_mutex_t mMx;

};

class CThread{
public:
    CThread(void *(*threadFunc) (void *)){mThreadFunc = threadFunc;};
    virtual void start(){
        mCancel = false;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&_mThread, &attr, mThreadFunc, this);
        pthread_attr_destroy(&attr);

    }
    virtual void stop() {
        mThreadMutex.acquire();
        mCancel = true;
        mThreadMutex.release();

        while (pthread_kill(_mThread, 0) == 0)
            usleep(10);
        pthread_join(_mThread, NULL);
    }

    virtual bool isCancel() {
        mThreadMutex.acquire();
        bool ret = mCancel;
        mThreadMutex.release();
        return ret;
    }

private:
    void *(*mThreadFunc) (void *);
    pthread_t _mThread;
    bool mCancel;

    CMutex mThreadMutex;
};



template <class B_Type> class CQueue: public std::deque<B_Type>, public CMutex {
};

typedef struct{
    uint64_t pts;
    void* data;
}CVideoFrame;

typedef struct{
    int32_t colorspace;
    int32_t width;
    int32_t height;
}CVideoFrameConfig;

typedef struct{
    int32_t size;
    void* data;
}CAudioFrame;

typedef struct{
    int32_t chanels;
    int32_t samplerate;
}CAudioFrameConfig;


typedef enum {
    EMPTY_MESSAGE = 0,
    AUDIO_PKT,
    VIDEO_PKT,
    AUDIO_FRAME,
    VIDEO_FRAME,
    AUDIO_CODEC_CONFIG,
    VIDEO_CODEC_CONFIG,
    AUDIO_RENDER_CONFIG,
    VIDEO_RENDER_CONFIG,
    FILE_EOF,
} MessageType;

struct CMessage{
    MessageType type;
    void* data;
};

void clearCMessage(MessageType type, void* data);
void clearCMessage(CMessage* msg);





#endif //SYSRAZPLAYER_UTILS_H
