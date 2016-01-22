//
// Created by ror13 on 13.12.15.
//


#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include <stdint.h>
#endif

#include "CPlayer.h"

#include <binder/ProcessState.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBufferGroup.h>
//#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <media/stagefright/ColorConverter.h>
#include <media/stagefright/foundation/ALooper.h>
#include <utils/List.h>
#include <new>
#include <map>
#include <mutex>
#include <queue>
#include <tuple>

extern "C" {
#include <stdio.h>
#include <pthread.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

}

#define MAX_QUEUE_SIZE 10
#define NATIVE_WINDOW_BUFFER_COUNT 1

using namespace android;

int64_t getTime(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}

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
        __android_log_write(ANDROID_LOG_ERROR, "stop", "1");
        mThreadMutex.acquire();
        __android_log_write(ANDROID_LOG_ERROR, "stop", "2");
        mCancel = true;
        mThreadMutex.release();
        __android_log_write(ANDROID_LOG_ERROR, "stop", "3");

        while (pthread_kill(_mThread, 0) == 0)
            sleep(1);
        pthread_join(_mThread, NULL);
        __android_log_write(ANDROID_LOG_ERROR, "stop", "4");
    }

    virtual bool isCancel() {
        mThreadMutex.acquire();
        bool ret = mCancel;
        mThreadMutex.release();
        return ret;
    }

protected:
    void *(*mThreadFunc) (void *);
    pthread_t _mThread;
    bool mCancel;
private:
    CMutex mThreadMutex;
};



template <class B_Type> class CQueue: public std::queue<B_Type>, public CMutex {
};




class CDemuxer : public CThread {
public:
    CDemuxer(const char * path, bool isNetwork = false);
    ~CDemuxer();
    const AVStream* getVideoStream(){return mVideoStream != -1 ? mFormatContext->streams[mVideoStream] : NULL;};
    const AVStream* getAudioStream(){return mAudioStream != -1 ? mFormatContext->streams[mAudioStream] : NULL;};
    CQueue <AVPacket*>* getVideoQueue(){return &mVideoQueue;};
    CQueue <AVPacket*>* getAudioQueue(){return &mAudioQueue;};
    void flush();
    bool isEof(){ return mEof;};

protected:
    static void * fileReading(void * baseObj);

    AVFormatContext* mFormatContext;
    int mVideoStream,
        mAudioStream;
    CQueue <AVPacket*> mVideoQueue,
                         mAudioQueue;
    pthread_t mThread;
    bool mIsNetwork;
    bool mEof;
};


CDemuxer::CDemuxer(const char * path, bool isNetwork):CThread(fileReading) {
    av_register_all();
    mVideoStream = -1;
    mAudioStream = -1;
    mEof = false;
    mFormatContext = avformat_alloc_context();
    avformat_network_init();


    AVDictionary *options = NULL;
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    //av_dict_set(&options, "thread_queue_size", "0", 0);
    //av_dict_set(&options, "reorder_queue_size", "0", 0);

    avformat_open_input(&mFormatContext, path, NULL, &options);
    av_dict_free(&options);
    avformat_find_stream_info(mFormatContext,NULL);

    for (int i = 0; i < mFormatContext->nb_streams; i++) {
        if (mFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            mVideoStream = i;
            continue;
        }
        if (mFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            mAudioStream = i;
            continue;
        }
    }
    mIsNetwork = isNetwork;
}

CDemuxer::~CDemuxer(){
    avformat_close_input(&mFormatContext);
};

void CDemuxer::flush(){
    mVideoQueue.acquire();
    while(!mVideoQueue.empty()){
        av_free_packet(mVideoQueue.front());
        mVideoQueue.pop();
    }
    mVideoQueue.release();

    mAudioQueue.acquire();
    while(!mAudioQueue.empty()){
        av_free_packet(mAudioQueue.front());
        mAudioQueue.pop();
    }
    mAudioQueue.release();

    avformat_flush(mFormatContext);
}

void* CDemuxer::fileReading(void * baseObj) {
    CDemuxer* self = (CDemuxer*) baseObj;

    if(self->mIsNetwork){
        self->flush();
    }
    for (;;) {

        for(;;) {
            if(self->isCancel()){
                self->flush();
                return NULL;
            }

            self->mAudioQueue.acquire();
            self->mVideoQueue.acquire();
            if(self->mVideoQueue.size() > MAX_QUEUE_SIZE ||
                    self->mAudioQueue.size() > MAX_QUEUE_SIZE) {
                self->mVideoQueue.release();
                self->mAudioQueue.release();
                usleep(1000000 / 60);
                continue;
            }
            self->mVideoQueue.release();
            self->mAudioQueue.release();
            break;
        }

        AVPacket* packet = new AVPacket;
        if(av_read_frame(self->mFormatContext, packet) < 0){
            self->mEof = true;
            delete  packet;
            break;
        }
        if (packet->stream_index == self->mAudioStream) {
            self->mAudioQueue.acquire();
            //self->mAudioQueue.push(packet);
            av_free_packet(packet);
            self->mAudioQueue.release();
            continue;
        }
        if (packet->stream_index == self->mVideoStream) {
            self->mVideoQueue.acquire();
            self->mVideoQueue.push(packet);
            self->mVideoQueue.release();
            continue;
        }
        // Free the packet that was allocated by av_read_frame
        //av_free_packet(&packet);
    }
    return NULL;
}



class CustomSource : public MediaSource {
public:
    CustomSource(AVCodecContext * codecContext, CQueue <AVPacket*>* inputQueu) {
        mConverter = NULL;
        mCodecContext = codecContext;
        mInputQueu = inputQueu;
        mFormat = new MetaData;
        size_t bufferSize = (mCodecContext->width * mCodecContext->height * 3) / 2;
        mGroup.add_buffer(new MediaBuffer(bufferSize));

        switch (mCodecContext->codec_id) {
            case CODEC_ID_H264:
                mConverter = av_bitstream_filter_init("h264_mp4toannexb");
                mFormat->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);
                if (mCodecContext->extradata[0] == 1) {
                    mFormat->setData(kKeyAVCC, kTypeAVCC, mCodecContext->extradata, mCodecContext->extradata_size);
                }
                break;
            case CODEC_ID_MPEG4:
                mFormat->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_MPEG4);
                /*if (mDataSource->iformat && mDataSource->iformat->name && !strcasecmp(mDataSource->iformat->name, FFMPEG_AVFORMAT_MOV)) {
                    MOVContext *mov = (MOVContext *)(mDataSource->priv_data);
                    if (mov->esds_data != NULL && mov->esds_size > 0 && mov->esds_size < 256) {
                        mFormat->setData(kKeyESDS, kTypeESDS, mov->esds_data, mov->esds_size);
                    }
                }*/
                break;
            case CODEC_ID_H263:
                mFormat->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_H263);
                break;
            default:
                break;
        }
        mFormat->setInt32(kKeyWidth, mCodecContext->width);
        mFormat->setInt32(kKeyHeight, mCodecContext->height);
    }

    virtual sp<MetaData> getFormat() {
        return mFormat;
    }

    virtual status_t start(MetaData *params) {
        return OK;
    }

    virtual status_t stop() {
        return OK;
    }

    virtual status_t read(MediaBuffer **buffer,
                          const MediaSource::ReadOptions *options) {
        status_t ret;
        for (;;) {
            mInputQueu->acquire();
            if (!mInputQueu->empty()) {
                break;
            }
            mInputQueu->release();

        }
        AVPacket * packet = mInputQueu->front();
        mInputQueu->pop();
        mInputQueu->release();
        //__android_log_print(ANDROID_LOG_INFO, "sometag", "paket size %d", packet->size);
        if (mConverter) {
            av_bitstream_filter_filter(mConverter, mCodecContext, NULL, &packet->data, &packet->size, packet->data, packet->size, packet->flags & AV_PKT_FLAG_KEY);
        }
        ret = mGroup.acquire_buffer(buffer);
        if (ret == OK) {
            memcpy((*buffer)->data(), packet->data, packet->size);
            (*buffer)->set_range(0, packet->size);
            (*buffer)->meta_data()->clear();
            (*buffer)->meta_data()->setInt32(kKeyIsSyncFrame, packet->flags & AV_PKT_FLAG_KEY);
            if(packet->pts>=0)
                (*buffer)->meta_data()->setInt64(kKeyTime, packet->pts);
            else
                (*buffer)->meta_data()->setInt64(kKeyTime, 0);
        }
        av_free_packet(packet);
        delete packet;
        return ret;
    }

    virtual ~CustomSource() {
        if (mConverter) {
            av_bitstream_filter_close(mConverter);
        }
    }
private:
    CQueue <AVPacket*>* mInputQueu;
    AVCodecContext *mCodecContext;
    AVBitStreamFilterContext *mConverter;
    MediaBufferGroup mGroup;
    sp<MetaData> mFormat;
};


class CDecoder : public CThread {
public:
    CDecoder(CDemuxer * demuxer);
    CQueue <MediaBuffer*>* getOutQueue(){return &mOutQueue;};
    void flush();
    void setNativeWindow(ANativeWindow* nativeWindow){mNativeWindow = nativeWindow;};
protected:
    static void * queueVideoDecoding(void * baseObj);
    CDemuxer * mDemuxer;
    ANativeWindow * mNativeWindow;
    CQueue <MediaBuffer*> mOutQueue;
};

CDecoder::CDecoder(CDemuxer * demuxer):CThread(queueVideoDecoding) {
    mDemuxer = demuxer;
    mNativeWindow = NULL;
}

void CDecoder::flush() {
    mOutQueue.acquire();
    while(!mOutQueue.empty()){
        mOutQueue.front()->release();
        mOutQueue.pop();
    }
    mOutQueue.release();
}




void * CDecoder::queueVideoDecoding(void * baseObj){
    CDecoder * self = (CDecoder*) baseObj;
    OMXClient client;
    client.connect();
    sp<MediaSource> videoSource = new CustomSource(self->mDemuxer->getVideoStream()->codec, self->mDemuxer->getVideoQueue());
    sp<MediaSource> videoDecoder = OMXCodec::Create(client.interface(), videoSource->getFormat(), \
               false, videoSource, NULL, OMXCodec::kOnlySubmitOneInputBufferAtOneTime, self->mNativeWindow);
               //false, videoSource, NULL, 0, self->mNativeWindow);
    videoDecoder->start();


    for (;;) {
        if(self->isCancel()){
            break;
        }
        MediaBuffer *videoBuffer;
        MediaSource::ReadOptions options;
        status_t err = videoDecoder->read(&videoBuffer, &options);
        if (err == OK) {
            if (videoBuffer->range_length() > 0) {

                self->mOutQueue.acquire();
                self->mOutQueue.push(videoBuffer);
                self->mOutQueue.release();

            } else {
                if(self->mDemuxer->isEof()) {
                    //videoBuffer->release();
                    break;
                }
            }
            //videoBuffer->release();
        }
    }

    self->flush();
    //videoSource.clear();
    videoDecoder->stop();
    //videoDecoder->clear();
    client.disconnect();
    client.disconnect();
    return NULL;

}

class CVideoRender : public CThread{
public:
    CVideoRender(CQueue <MediaBuffer*>* inputQueue, ANativeWindow * nativeWindow);
protected:
    static void * queueVideoRendering(void * baseObj);
    CQueue <MediaBuffer*> * mInputQueue;
    ANativeWindow * mNativeWindow;
};

CVideoRender::CVideoRender(CQueue <MediaBuffer*>* inputQueue, ANativeWindow * nativeWindow) :CThread(queueVideoRendering){
    mInputQueue = inputQueue;
    mNativeWindow = nativeWindow;
    ANativeWindow_setBuffersGeometry(mNativeWindow, 1920, 1080, WINDOW_FORMAT_RGBA_8888);
}

void * CVideoRender::queueVideoRendering(void * baseObj){
    CVideoRender* self = (CVideoRender*) baseObj;
    //native_window_set_buffer_count(self->mNativeWindow,NATIVE_WINDOW_BUFFER_COUNT);
    int64_t startTime = 0;
    int64_t lastPts = 0;
    for(;;){
        if(self->isCancel()){
            break;
        }

        MediaBuffer* mediaBuffer = NULL;
        self->mInputQueue->acquire();
        if(!self->mInputQueue->empty()){
            mediaBuffer = self->mInputQueue->front();
            self->mInputQueue->pop();
        }
        self->mInputQueue->release();
        if(!mediaBuffer){
            continue;
        }
        sp<MetaData> metaData = mediaBuffer->meta_data();
        int64_t currPts = 0;
        int64_t timeDelay;
        metaData->findInt64(kKeyTime, &currPts);
        if(startTime == 0) {
            lastPts = currPts;
            startTime = getTime();
        }else {
            timeDelay = (currPts - lastPts) * 1000 - (getTime() - startTime);
            lastPts = currPts;
            if (timeDelay <= 0) {
                startTime = 0;
                mediaBuffer->release();
               // __android_log_write(ANDROID_LOG_ERROR, "1", "------------------------");
                continue;
            }

            if (timeDelay > 0) {
                usleep(timeDelay-10);
            }
        }

        //native_window_set_buffers_timestamp(self->mNativeWindow, 1000 *1000 *1000);
        self->mNativeWindow->queueBuffer(self->mNativeWindow, mediaBuffer->graphicBuffer().get(),-1);
        mediaBuffer->release();

    }
    return NULL;
}


class CPlayer {
public:
    void open(ANativeWindow* nativeWindow, const char * filename, bool isStream){
        demuxer = new CDemuxer(filename,false);
        decoder = new CDecoder(demuxer);
        videoRender = new CVideoRender(decoder->getOutQueue(),nativeWindow);
        decoder->setNativeWindow(nativeWindow);
    }
    void close(){
        __android_log_write(ANDROID_LOG_ERROR, "close()", "1");
        if(isPlaying){
            stop();
        }
        __android_log_write(ANDROID_LOG_ERROR, "close()", "2");
        if(demuxer != NULL){
            delete demuxer;
        }
        __android_log_write(ANDROID_LOG_ERROR, "close()", "3");
        if(decoder != NULL){
            delete decoder;
        }
        __android_log_write(ANDROID_LOG_ERROR, "close()", "4");
        if(videoRender != NULL){
            delete videoRender;
        }
        __android_log_write(ANDROID_LOG_ERROR, "close()", "5");
    };

    void start(){
        demuxer->start();
        decoder->start();
        videoRender->start();
        isPlaying = true;
    }
    void stop(){
        videoRender->stop();
        decoder->stop();
        demuxer->stop();
        isPlaying = false;
    }


    CPlayer(){
        demuxer = NULL;
        decoder = NULL;
        videoRender = NULL;
        isPlaying = false;
    }

    ~CPlayer(){
        __android_log_write(ANDROID_LOG_ERROR, "~CPlayer()", "1");
            close();
        __android_log_write(ANDROID_LOG_ERROR, "~CPlayer()", "2");
    };
private:
    CDemuxer* demuxer;
    CDecoder* decoder;
    CVideoRender* videoRender;
    bool isPlaying;
};

extern "C" {


jfieldID
getJavaPointerToPlayer(JNIEnv *env, jobject obj){
    jclass cls = env->GetObjectClass(obj);
    return env->GetFieldID(cls, "pointerToPlayer", "J");
}



void
Java_com_ror13_sysrazplayer_CPlayer_open(JNIEnv *env, jobject thiz, jstring path, jobject surface, jboolean isStream){
    const char* filename = (env)->GetStringUTFChars(path, 0);
    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env,surface);

    CPlayer* player = (CPlayer*) env->GetLongField(thiz, getJavaPointerToPlayer(env, thiz));
    if(player != NULL){
        env->SetLongField(thiz, getJavaPointerToPlayer(env, thiz), (jlong)NULL);
        delete player;
        __android_log_write(ANDROID_LOG_ERROR, "2", "delete playe");
    }

    player = new CPlayer();
    player->open(nativeWindow,  filename, (bool)isStream);
    env->SetLongField(thiz, getJavaPointerToPlayer(env, thiz), (jlong)player);

}

void
Java_com_ror13_sysrazplayer_CPlayer_close(JNIEnv *env, jobject thiz){
    CPlayer* player = (CPlayer*) env->GetLongField(thiz, getJavaPointerToPlayer(env, thiz));
    delete player;
    env->SetLongField(thiz, getJavaPointerToPlayer(env, thiz), (jlong)NULL);
}
void
Java_com_ror13_sysrazplayer_CPlayer_start(JNIEnv *env, jobject thiz){
    CPlayer* player = (CPlayer*) env->GetLongField(thiz, getJavaPointerToPlayer(env, thiz));
    player->start();
}

void
Java_com_ror13_sysrazplayer_CPlayer_stop(JNIEnv *env, jobject thiz){
    CPlayer* player = (CPlayer*) env->GetLongField(thiz, getJavaPointerToPlayer(env, thiz));
    player->stop();
}



jstring
Java_com_ror13_sysrazplayer_CFfmpeg_tst( JNIEnv* env, jobject thiz ){
    #if defined(__arm__)
    #if defined(__ARM_ARCH_7A__)
      #if defined(__ARM_NEON__)
        #if defined(__ARM_PCS_VFP)
          #define ABI "armeabi-v7a/NEON (hard-float)"
        #else
          #define ABI "armeabi-v7a/NEON"
        #endif
      #else
        #if defined(__ARM_PCS_VFP)
          #define ABI "armeabi-v7a (hard-float)"
        #else
          #define ABI "armeabi-v7a"
        #endif
      #endif
    #else
     #define ABI "armeabi"
    #endif
    #elif defined(__i386__)
    #define ABI "x86"
    #elif defined(__x86_64__)
    #define ABI "x86_64"
    #elif defined(__mips64)  /* mips64el-* toolchain defines __mips__ too */
    #define ABI "mips64"
    #elif defined(__mips__)
    #define ABI "mips"
    #elif defined(__aarch64__)
    #define ABI "arm64-v8a"
    #else
    #define ABI "unknown"
    #endif

    return (env)->NewStringUTF( "Hello from JNI !  Compiled with ABI " ABI ".");
}

}