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
#include "CGlesRender.h"

#include <binder/ProcessState.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <media/stagefright/ColorConverter.h>
#include <media/stagefright/foundation/ALooper.h>
#include <utils/List.h>
#include <new>
#include <string>
#include <map>
#include <mutex>
#include <deque>
#include <tuple>

extern "C" {
#include <stdio.h>
#include <pthread.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

}



#define kKeyTimePTS 666
#define NATIVE_WINDOW_BUFFER_COUNT 1

using namespace android;

unsigned long GFps_GetCurFps();

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
    bool isFrist;
    int32_t colorspace;
    int32_t width;
    int32_t height;
    uint64_t pts;
    void * data;
}CVideoFrame;

class CDemuxer : public CThread {
public:
    CDemuxer();
    ~CDemuxer();
    void openFile(const char * path);
    void closeFile();
    const AVStream* getVideoStream(){return mVideoStream != -1 ? mFormatContext->streams[mVideoStream] : NULL;}
    const AVStream* getAudioStream(){return mAudioStream != -1 ? mFormatContext->streams[mAudioStream] : NULL;}
    CQueue <AVPacket*>* getVideoQueue(){return &mVideoQueue;};
    CQueue <AVPacket*>* getAudioQueue(){return &mAudioQueue;};
    void configure(bool isLoop = false, bool isSkipPacket = false, unsigned int packetBufferSize = 10, const char * rtspProtocolType = "tcp");
    void setFlushOnOpen(bool flushOnOpen){mFlushOnOpen = flushOnOpen;}
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
    bool mEof;
    bool mIsSkipPacket;
    unsigned int mPacketBufferSize;
    std::string  mRtspProtocolType;
    bool mFlushOnOpen;
    bool mIsLoop;
};



CDemuxer::CDemuxer():CThread(fileReading) {
    av_register_all();
    mVideoStream = -1;
    mAudioStream = -1;
    mEof = false;
    mFormatContext = NULL;
    avformat_network_init();
    configure();
}

CDemuxer::~CDemuxer(){
    flush();
    closeFile();
    avformat_network_deinit();
};


void CDemuxer::closeFile(){
    mVideoStream = -1;
    mAudioStream = -1;
    avformat_flush(mFormatContext);
    if(mFormatContext)
        avformat_close_input(&mFormatContext);
    avformat_free_context(mFormatContext);
    mFormatContext = NULL;

};



void CDemuxer::openFile(const char * path){
    mFormatContext = avformat_alloc_context();
    AVDictionary *options = NULL;
    av_dict_set(&options, "rtsp_transport", mRtspProtocolType.c_str(), 0);
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

    if(mFlushOnOpen){
        avformat_flush(mFormatContext);
    }
}

void CDemuxer::configure(bool isLoop, bool isSkipPacket, unsigned int packetBufferSize, const char * rtspProtocolType){
    mIsSkipPacket = isSkipPacket;
    mPacketBufferSize = packetBufferSize;
    mRtspProtocolType = rtspProtocolType;
    mIsLoop = isLoop;
}

void CDemuxer::flush(){
    mVideoQueue.acquire();
    while(!mVideoQueue.empty()){
        av_free_packet(mVideoQueue.front());
        mVideoQueue.pop_front();
    }
    mVideoQueue.release();

    mAudioQueue.acquire();
    while(!mAudioQueue.empty()){
        av_free_packet(mAudioQueue.front());
        mAudioQueue.pop_front();
    }
    mAudioQueue.release();

    avformat_flush(mFormatContext);
}

void* CDemuxer::fileReading(void * baseObj) {
    CDemuxer* self = (CDemuxer*) baseObj;

    for (;;) {

        for(;;) {
            if(self->isCancel()){
                self->flush();
                return NULL;
            }

            self->mAudioQueue.acquire();
            self->mVideoQueue.acquire();
            if(self->mVideoQueue.size() > self->mPacketBufferSize ||
                    self->mAudioQueue.size() > self->mPacketBufferSize) {
                if(self->mIsSkipPacket){
                    if(!self->mVideoQueue.empty()){
                        av_free_packet(self->mVideoQueue.back());
                        self->mVideoQueue.pop_back();
                    }
                    if(!self->mAudioQueue.empty()){
                        av_free_packet(self->mAudioQueue.back());
                        self->mAudioQueue.pop_back();
                    }
                }else{
                    self->mVideoQueue.release();
                    self->mAudioQueue.release();
                    usleep(1000000 / 60);
                    continue;
                }
            }
            self->mVideoQueue.release();
            self->mAudioQueue.release();
            break;
        }

        AVPacket* packet = new AVPacket;
        if(av_read_frame(self->mFormatContext, packet) < 0){
            delete  packet;
            if(self->mIsLoop){
                std::string fileNmae = self->mFormatContext->filename;
                self->closeFile();
                self->openFile(fileNmae.c_str());
                continue;
            }
            self->mEof = true;
            break;
        }
        if (packet->stream_index == self->mAudioStream) {
            self->mAudioQueue.acquire();
            //self->mAudioQueue.push_back(packet);
            av_free_packet(packet);
            self->mAudioQueue.release();
            continue;
        }
        if (packet->stream_index == self->mVideoStream) {
            self->mVideoQueue.acquire();
            self->mVideoQueue.push_back(packet);
            self->mVideoQueue.release();
            continue;
        }
    }
    return NULL;
}



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

CVideoRender::CVideoRender(CQueue <CVideoFrame*>* inputQueue, ANativeWindow * nativeWindow) :CThread(queueVideoRendering){
    mInputQueue = inputQueue;
    mNativeWindow = nativeWindow;
    int width =  ANativeWindow_getWidth(mNativeWindow);
    int height = ANativeWindow_getHeight(mNativeWindow);
    ANativeWindow_setBuffersGeometry(mNativeWindow, width, height, WINDOW_FORMAT_RGBX_8888);
    mIsUsingPts = true;
    mRenderOpenGles = NULL;
}



void CVideoRender::makeRender(){
    //native_window_set_buffer_count(self->mNativeWindow,NATIVE_WINDOW_BUFFER_COUNT);
    if(mWindowType == CVideoRender::GLES_WINDOW && !mRenderOpenGles){
        mRenderOpenGles = new CGlesRender(mNativeWindow);
    }
}

void CVideoRender::initializeRender(int colorspace, int frameWidth, int frameHeight){
    mFrameWidth = frameWidth;
    mFrameHeight = frameHeight;
    if(mWindowType == CVideoRender::GLES_WINDOW) {
        mRenderOpenGles->clear();
        COLORSPACE colorGles;
        switch (colorspace) {
            case OMX_QCOM_COLOR_FormatYVU420SemiPlanar:
            case OMX_COLOR_FormatYUV420SemiPlanar:
                colorGles = COLORSPACE::COLOR_NV12;
                mColorspace = AV_PIX_FMT_NV12;
                break;
            case OMX_COLOR_FormatYCbYCr:
            case OMX_COLOR_FormatCbYCrY:
                colorGles = COLORSPACE::COLOR_YUYV;
                mColorspace = AV_PIX_FMT_YUYV422;
                break;
            case OMX_COLOR_FormatYUV420Planar:
            case OMX_COLOR_FormatYUV420PackedPlanar:
            default:
                colorGles = COLORSPACE::COLOR_YUV420P;
                mColorspace = AV_PIX_FMT_YUV420P;
                break;
        }
        mRenderOpenGles->initialize(colorGles, mFrameWidth, mFrameHeight);
    }
}

void CVideoRender::desroyRender(){
    if(mRenderOpenGles){
        mRenderOpenGles->clear();
        delete mRenderOpenGles;
        mRenderOpenGles = NULL;
    }
}

void CVideoRender::drawFrame(void * data){
    MediaBuffer *videoBuffer = (MediaBuffer *)data;
    sp<MetaData> metaData = videoBuffer->meta_data();
    if(mWindowType == CVideoRender::NATIVE_WINDOW) {
        mNativeWindow->queueBuffer(mNativeWindow,
                                   videoBuffer->graphicBuffer().get(), -1);
        metaData->setInt32(kKeyRendered, 1);
    }

    if(mWindowType == CVideoRender::GLES_WINDOW){
        AVPicture picture;
        avpicture_fill(&picture, (uint8_t*)videoBuffer->data(), mColorspace, mFrameWidth, mFrameHeight);
        mRenderOpenGles->draw((void**)picture.data);
        mRenderOpenGles->swap();
    }
    videoBuffer->release();
    //__android_log_print(ANDROID_LOG_INFO, "FPS", " %ld", GFps_GetCurFps());
}

void * CVideoRender::queueVideoRendering(void* baseObj){
    CVideoRender* self = (CVideoRender*) baseObj;
    self->makeRender();
    int64_t startTime = -1;
    int64_t lastPts = 0;
    for(;;){
        if(self->isCancel()){
            break;
        }

        CVideoFrame* videoFrame = NULL;
        self->mInputQueue->acquire();
        if(!self->mInputQueue->empty()){
            videoFrame = self->mInputQueue->front();
            self->mInputQueue->pop_front();
        }
        self->mInputQueue->release();
        if(!videoFrame){
            continue;
        }


        if(self->mIsUsingPts) {
            MediaBuffer *videoBuffer = (MediaBuffer *)videoFrame->data;
            sp<MetaData> metaData = videoBuffer->meta_data();

            int64_t currPts = 0;
            int64_t timeDelay;
            metaData->findInt64(kKeyTimePTS, &currPts);
            if (startTime < 0) {
                lastPts = currPts;
                startTime = getTime();
            } else {
                timeDelay = (currPts - lastPts) * 1000 - (getTime() - startTime);
                lastPts = currPts;
                if (timeDelay <= 0) {
                    startTime = 0;
                    videoBuffer->release();
                    // v
                    continue;
                }

                if (timeDelay > 0) {
                    usleep(timeDelay - 10);
                }
            }
        }

        if(videoFrame->isFrist){
            self->initializeRender(videoFrame->colorspace, videoFrame->width, videoFrame->height);
        }
        self->drawFrame(videoFrame->data);



        //__android_log_print(ANDROID_LOG_INFO, "FPS", " %ld", GFps_GetCurFps());

        delete  videoFrame;


    }
    self->desroyRender();
    return NULL;
}





class CustomSource : public MediaSource {
public:
    CustomSource(CDemuxer * demuxer) {
        mDemuxer = demuxer;
        mConverter = NULL;
        AVCodecContext* codecContext = mDemuxer->getVideoStream()->codec;
        mInputQueu = mDemuxer->getVideoQueue();
        mFormat = new MetaData;
        size_t bufferSize = (codecContext->width * codecContext->height * 3) / 2;
        mGroup.add_buffer(new MediaBuffer(bufferSize));

        switch (codecContext->codec_id) {
            case CODEC_ID_H264:
                mFormat->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);
                mConverter = av_bitstream_filter_init("h264_mp4toannexb");
                if (codecContext->extradata[0] == 1) {
                    mFormat->setData(kKeyAVCC, kTypeAVCC, codecContext->extradata, codecContext->extradata_size);
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
        mFormat->setInt32(kKeyWidth, codecContext->width);
        mFormat->setInt32(kKeyHeight, codecContext->height);
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
        AVPacket packetOut= *packet;


        mInputQueu->pop_front();
        mInputQueu->release();

        if (mConverter) {
            while(!mDemuxer->getVideoStream())
                usleep(1);
            av_bitstream_filter_filter(mConverter, mDemuxer->getVideoStream()->codec, NULL, &packetOut.data, &packetOut.size, packet->data, packet->size, packet->flags & AV_PKT_FLAG_KEY);
        }

        //__android_log_print(ANDROID_LOG_INFO, "sometag", "paket size %d", packet->size);
        ret = mGroup.acquire_buffer(buffer);
        if (ret == OK) {
            memcpy((*buffer)->data(), packetOut.data, packetOut.size);
            (*buffer)->set_range(0, packetOut.size);
            (*buffer)->meta_data()->clear();
            (*buffer)->meta_data()->setInt32(kKeyIsSyncFrame, packet->flags & AV_PKT_FLAG_KEY);
            (*buffer)->meta_data()->setInt64(kKeyTime, 0);
            if(packet->pts != AV_NOPTS_VALUE)
                (*buffer)->meta_data()->setInt64(kKeyTimePTS, packet->pts);
            else
                (*buffer)->meta_data()->setInt64(kKeyTimePTS, packet->dts);
        }
        if(packetOut.data != packet->data){
            av_free(packetOut.data);
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
    CDemuxer *mDemuxer;

    MediaBufferGroup mGroup;
    sp<MetaData> mFormat;

    AVBitStreamFilterContext* mConverter;

};



class CDecoder : public CThread {
public:
    CDecoder(CDemuxer * demuxer);
    CQueue <CVideoFrame*>* getOutQueue(){return &mOutQueue;}
    void flush();
    void setFlushDemuxer(bool needFlush){mIsFlushDemuxer = needFlush;}
    void setNativeWindow(ANativeWindow* nativeWindow){mNativeWindow = nativeWindow;}
    void setVideoRender(CVideoRender* videoRender = NULL){mVideoRender = videoRender;}
protected:
    static void * queueVideoDecoding(void * baseObj);
    CDemuxer * mDemuxer;
    ANativeWindow * mNativeWindow;
    CQueue <CVideoFrame*> mOutQueue;
    CVideoRender* mVideoRender;
    bool mIsFlushDemuxer;
};

CDecoder::CDecoder(CDemuxer * demuxer):CThread(queueVideoDecoding) {
    mDemuxer = demuxer;
    mNativeWindow = NULL;
    mIsFlushDemuxer = false;
    mVideoRender = NULL;
}

void CDecoder::flush() {
    mOutQueue.acquire();
    while(!mOutQueue.empty()){
        MediaBuffer* videoBuffer = (MediaBuffer*)mOutQueue.front()->data;
        videoBuffer->release();
        delete (mOutQueue.front());
        mOutQueue.pop_front();
    }
    mOutQueue.release();
}




void * CDecoder::queueVideoDecoding(void * baseObj){
    CDecoder * self = (CDecoder*) baseObj;
    bool isFirstPacket = true;
    OMXClient client;
    client.connect();
    sp<MediaSource> videoSource = new CustomSource(self->mDemuxer);
    sp<MediaSource> videoDecoder = OMXCodec::Create(client.interface(), videoSource->getFormat(), \
               false, videoSource, NULL, OMXCodec::kHardwareCodecsOnly | OMXCodec::kOnlySubmitOneInputBufferAtOneTime,
               self->mNativeWindow);
    videoDecoder->start();
    int32_t frameWidth = -1;
    int32_t frameHeight = -1;
    videoSource->getFormat()->findInt32(kKeyWidth, &frameWidth);
    videoSource->getFormat()->findInt32(kKeyHeight, &frameHeight);
    int32_t colorFormat = -1;
    videoDecoder->getFormat()->findInt32(kKeyColorFormat, &colorFormat);

    if(self->mVideoRender != NULL){
        self->mVideoRender->makeRender();
        self->mVideoRender->initializeRender(colorFormat, frameWidth, frameHeight);
    }

    if(self->mIsFlushDemuxer){
        self->mDemuxer->flush();
    }

    for (;;) {
        if(self->isCancel()){
            break;
        }
        MediaBuffer *videoBuffer;
        MediaSource::ReadOptions options;
        status_t err = videoDecoder->read(&videoBuffer, &options);
        if (err != OK) {
            continue;
        }

        if (videoBuffer->range_length() <= 0) {
            if(self->mDemuxer->isEof()) {
                break;
            }
            continue;
        }

        if(self->mVideoRender != NULL){
            self->mVideoRender->drawFrame((void*)videoBuffer);
        }else{
            CVideoFrame * frame = new CVideoFrame();
            memset(frame,0x0,sizeof(CVideoFrame));
            frame->data = videoBuffer;
            frame->pts = 0;
            if(isFirstPacket) {
                isFirstPacket = false;
                frame->colorspace = colorFormat;
                frame->width = frameWidth;
                frame->height = frameHeight;
                frame->isFrist = true;
            }

            self->mOutQueue.acquire();
            self->mOutQueue.push_back(frame);
            self->mOutQueue.release();
        }

    }

    self->flush();
    videoDecoder->stop();
    client.disconnect();
    if(self->mVideoRender != NULL){
        self->mVideoRender->desroyRender();
    }
    return NULL;

}



class CPlayer {
public:
    enum {
        OPT_URI = 0,
        OPT_RTSP_PROTOCOL,
        OPT_PACKET_BUFFER_SIZE,
        OPT_IS_FLUSH,
        OPT_IS_MAX_FPS,
        OPT_IS_SKIP_PACKET,
        OPT_IS_LOOP_PLAYING,
        OPT_IS_WINDOW_NATIVE,
        OPT_IS_WINDOW_GLES,
        OPT_IS_VIDEO_QUEUE
    };
    typedef struct {
        std::string uri;
        std::string rtspProtocol;
        int packetBufferSize;
        bool isFlush;
        bool isMaxFps;
        bool isSkipPacket;
        bool isLoopPlaying;
        bool isWindowNative;
        bool isWindowGles;
        bool isVideoQueue;

    } CplayerConfig;
    void open(ANativeWindow* nativeWindow, CplayerConfig * conf){
        demuxer = new CDemuxer();
        demuxer->configure(conf->isLoopPlaying, conf->isSkipPacket, conf->packetBufferSize,conf->rtspProtocol.c_str());
        demuxer->setFlushOnOpen(conf->isFlush);
        demuxer->openFile(conf->uri.c_str());

        decoder = new CDecoder(demuxer);
        decoder->setFlushDemuxer(conf->isFlush);

        videoRender = new CVideoRender(decoder->getOutQueue(),nativeWindow);
        bool isUsingPts = !conf->isMaxFps;
        videoRender->setUsingPts(isUsingPts);
        if(conf->isWindowNative){
            decoder->setNativeWindow(nativeWindow);
            videoRender->setUsingWindowType(CVideoRender::NATIVE_WINDOW);
        }
        if(conf->isWindowGles){
            decoder->setNativeWindow(NULL);
            videoRender->setUsingWindowType(CVideoRender::GLES_WINDOW);
        }

        mIsVideoQueue = conf->isVideoQueue;
        if(mIsVideoQueue == false){
            decoder->setVideoRender(videoRender);
        }


    }
    void close(){
        if(mIsPlaying){
            stop();
        }
        if(demuxer != NULL){
            delete demuxer;
        }
        if(decoder != NULL){
            delete decoder;
        }
        if(videoRender != NULL){
            delete videoRender;
        }
    };

    void start(){
        demuxer->start();
        decoder->start();
        if(mIsVideoQueue){
            videoRender->start();
        }

        mIsPlaying = true;
    }
    void stop(){
        if(mIsVideoQueue){
            videoRender->stop();
        }
        decoder->stop();
        demuxer->stop();
        mIsPlaying = false;
    }


    CPlayer(){
        demuxer = NULL;
        decoder = NULL;
        videoRender = NULL;
        mIsVideoQueue = true;
        mIsPlaying = false;
    }

    ~CPlayer(){
            close();
    };
private:
    CDemuxer* demuxer;
    CDecoder* decoder;
    CVideoRender* videoRender;
    bool mIsPlaying;
    bool mIsVideoQueue;
};

extern "C" {


jfieldID
getJavaPointerToPlayer(JNIEnv *env, jobject obj){
    jclass cls = env->GetObjectClass(obj);
    return env->GetFieldID(cls, "pointerToPlayer", "J");
}



void
Java_com_ror13_sysrazplayer_CPlayer_open(JNIEnv *env, jobject thiz, jobject config, jobject surface){
    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env,surface);

    CPlayer* player = (CPlayer*) env->GetLongField(thiz, getJavaPointerToPlayer(env, thiz));
    if(player != NULL) {
        env->SetLongField(thiz, getJavaPointerToPlayer(env, thiz), (jlong) NULL);
        delete player;
    }

    CPlayer::CplayerConfig cplayerConfig;
    jmethodID getValObj = env->GetMethodID(env->GetObjectClass(config), "getValObj", "(I)Ljava/lang/Object;");
    jmethodID getValInt = env->GetMethodID(env->GetObjectClass(config), "getValInt", "(I)I");
    jmethodID getValBool = env->GetMethodID(env->GetObjectClass(config), "getValBool", "(I)Z");

    jobject uri = env->CallObjectMethod(config,getValObj,CPlayer::OPT_URI);
    cplayerConfig.uri = (env)->GetStringUTFChars((jstring)uri, 0);

    jobject rtspProtocol = env->CallObjectMethod(config,getValObj,CPlayer::OPT_RTSP_PROTOCOL);
    cplayerConfig.rtspProtocol = (env)->GetStringUTFChars((jstring)rtspProtocol, 0);



    cplayerConfig.packetBufferSize = (jint) env->CallIntMethod(config,getValInt,CPlayer::OPT_PACKET_BUFFER_SIZE);


    cplayerConfig.isFlush = (jboolean) env->CallBooleanMethod(config,getValBool,CPlayer::OPT_IS_FLUSH);
    cplayerConfig.isMaxFps = (jboolean) env->CallBooleanMethod(config,getValBool,CPlayer::OPT_IS_MAX_FPS);
    cplayerConfig.isSkipPacket = (jboolean) env->CallBooleanMethod(config,getValBool,CPlayer::OPT_IS_SKIP_PACKET);
    cplayerConfig.isLoopPlaying = (jboolean) env->CallBooleanMethod(config,getValBool,CPlayer::OPT_IS_LOOP_PLAYING);
    cplayerConfig.isWindowNative = (jboolean) env->CallBooleanMethod(config,getValBool,CPlayer::OPT_IS_WINDOW_NATIVE);
    cplayerConfig.isWindowGles = (jboolean) env->CallBooleanMethod(config,getValBool,CPlayer::OPT_IS_WINDOW_GLES);
    cplayerConfig.isVideoQueue = (jboolean) env->CallBooleanMethod(config,getValBool,CPlayer::OPT_IS_VIDEO_QUEUE);


    player = new CPlayer();
    player->open(nativeWindow,  &cplayerConfig);
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


#include <time.h>

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