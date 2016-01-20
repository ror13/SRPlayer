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

#include "CFfmpeg.h"

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
#define NATIVE_WINDOW_BUFFER_COUNT 3

using namespace android;

int64_t getTime(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}

class JFFMutex{
public:
    JFFMutex(){mMx = PTHREAD_MUTEX_INITIALIZER;};
    void acquire(){pthread_mutex_lock(&this->mMx);};
    void release(){pthread_mutex_unlock(&this->mMx);};
protected:
    pthread_mutex_t mMx;

};

class JFFThread{
public:
    JFFThread(void *(*threadFunc) (void *)){mThreadFunc = threadFunc;};
    virtual void start(){
        pthread_create(&mThread, NULL, mThreadFunc, this);
    }

protected:
    void *(*mThreadFunc) (void *);
    pthread_t mThread;
};



template <class B_Type> class JFFQueue: public std::queue<B_Type>, public JFFMutex {
};




class JFFDemuxer : public JFFMutex, public JFFThread {
public:
    JFFDemuxer(const char * path, bool isNetwork = false);
    ~JFFDemuxer();
    const AVStream* getVideoStream(){return mVideoStream != -1 ? mFormatContext->streams[mVideoStream] : NULL;};
    const AVStream* getAudioStream(){return mAudioStream != -1 ? mFormatContext->streams[mAudioStream] : NULL;};
    JFFQueue <AVPacket*>* getVideoQueue(){return &mVideoQueue;};
    JFFQueue <AVPacket*>* getAudioQueue(){return &mAudioQueue;};
    bool isEof(){ return mEof;};

protected:
    static void * fileReading(void * baseObj);

    AVFormatContext* mFormatContext;
    int mVideoStream,
        mAudioStream;
    JFFQueue <AVPacket*> mVideoQueue,
                         mAudioQueue;
    pthread_t mThread;
    bool mIsNetwork;
    bool mEof;
};


JFFDemuxer::JFFDemuxer(const char * path, bool isNetwork):JFFThread(fileReading) {
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

JFFDemuxer::~JFFDemuxer(){
    avformat_close_input(&mFormatContext);
};

void* JFFDemuxer::fileReading(void * baseObj) {
    JFFDemuxer* self = (JFFDemuxer*) baseObj;

    if(self->mIsNetwork){
        avformat_flush(self->mFormatContext);
    }
    for (;;) {

        for(;;) {
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
        //self->acquire();
        if(av_read_frame(self->mFormatContext, packet) < 0){
            self->mEof = true;
            self->release();
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
        //self->release();
        // Free the packet that was allocated by av_read_frame
        //av_free_packet(&packet);
    }
}



class CustomSource : public MediaSource {
public:
    CustomSource(AVCodecContext * codecContext, JFFQueue <AVPacket*>* inputQueu) {
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
    JFFQueue <AVPacket*>* mInputQueu;
    AVCodecContext *mCodecContext;
    AVBitStreamFilterContext *mConverter;
    MediaBufferGroup mGroup;
    sp<MetaData> mFormat;
};


class JFFDecoder : public JFFThread {
public:
    JFFDecoder(JFFDemuxer * demuxer);
    JFFQueue <MediaBuffer*>* getOutQueue(){return &mOutQueue;};
    void setNativeWindow(ANativeWindow* nativeWindow){mNativeWindow = nativeWindow;};
protected:
    static void * queueVideoDecoding(void * baseObj);
    JFFDemuxer * mDemuxer;
    ANativeWindow * mNativeWindow;
    JFFQueue <MediaBuffer*> mOutQueue;
};

JFFDecoder::JFFDecoder(JFFDemuxer * demuxer):JFFThread(queueVideoDecoding) {
    mDemuxer = demuxer;
    mNativeWindow = NULL;
}





void * JFFDecoder::queueVideoDecoding(void * baseObj){
    JFFDecoder * self = (JFFDecoder*) baseObj;
    OMXClient client;
    client.connect();
    sp<MediaSource> videoSource = new CustomSource(self->mDemuxer->getVideoStream()->codec, self->mDemuxer->getVideoQueue());
    sp<MediaSource> videoDecoder = OMXCodec::Create(client.interface(), videoSource->getFormat(), \
               false, videoSource, NULL, OMXCodec::kOnlySubmitOneInputBufferAtOneTime, self->mNativeWindow);
    videoDecoder->start();


    for (;;) {
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
    //videoSource.clear();
    videoDecoder->stop();
    //videoDecoder->clear();
    client.disconnect();

}

class JFFVideoRender : public JFFThread{
public:
    JFFVideoRender(JFFQueue <MediaBuffer*>* inputQueue, ANativeWindow * nativeWindow);
protected:
    static void * queueVideoRendering(void * baseObj);
    JFFQueue <MediaBuffer*> * mInputQueue;
    ANativeWindow * mNativeWindow;
};

JFFVideoRender::JFFVideoRender(JFFQueue <MediaBuffer*>* inputQueue, ANativeWindow * nativeWindow) :JFFThread(queueVideoRendering){
    mInputQueue = inputQueue;
    mNativeWindow = nativeWindow;
    ANativeWindow_setBuffersGeometry(mNativeWindow, 1920, 1080, WINDOW_FORMAT_RGBA_8888);
}

void * JFFVideoRender::queueVideoRendering(void * baseObj){
    JFFVideoRender* self = (JFFVideoRender*) baseObj;
    native_window_set_buffer_count(self->mNativeWindow,NATIVE_WINDOW_BUFFER_COUNT);
    int64_t startTime = 0;
    int64_t startPts = 0;
    for(;;){
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
        int64_t timeUs = 0;
        int64_t timeDelay;
        metaData->findInt64(kKeyTime, &timeUs);
        if(startTime == 0) {
            startPts = timeUs;
            startTime = getTime();
            __android_log_write(ANDROID_LOG_ERROR, "0", "------------------------");
        }else {
            timeDelay = (timeUs - startPts) * 1000 - (getTime() - startTime);
            if (timeDelay <= 0) {
                startTime = 0;
                mediaBuffer->release();
                __android_log_write(ANDROID_LOG_ERROR, "1", "------------------------");
                continue;
            }

            if (timeDelay > 0) {
                //__android_log_print(ANDROID_LOG_INFO, "sometag", "time delay %lld   time %lld   pts  %lld", timeDelay,getTime() - startTime, (timeUs - startPts) * 1000);
                //usleep(timeDelay);
            }
        }

        //native_window_set_buffers_timestamp(self->mNativeWindow, timeUs * 1000);
        self->mNativeWindow->queueBuffer(self->mNativeWindow, mediaBuffer->graphicBuffer().get(),-1);
        mediaBuffer->release();

    }
}

/*
class CustomSource : public MediaSource {
public:
    CustomSource(const char *videoPath) {
        av_register_all();

        mDataSource = avformat_alloc_context();
        avformat_open_input(&mDataSource, videoPath, NULL, NULL);
        for (int i = 0; i < mDataSource->nb_streams; i++) {
            if (mDataSource->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
                mVideoIndex = i;
                break;
            }
        }
        mCodecContext = mDataSource->streams[mVideoIndex]->codec;

        size_t bufferSize = (mCodecContext->width * mCodecContext->height * 3) / 2;
        mGroup.add_buffer(new MediaBuffer(bufferSize));
        mFormat = new MetaData;

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
                }/
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
        AVPacket *packet;
        while (!found) {
            ret = av_read_frame(mDataSource, &packet);
            if (ret < 0) {
                return ERROR_END_OF_STREAM;
            }

            if (packet.stream_index == mVideoIndex) {
                if (1) {
                    av_bitstream_filter_filter(mConverter, mCodecContext, NULL, &packet.data, &packet.size, packet.data, packet.size, packet.flags & AV_PKT_FLAG_KEY);
                }
                ret = mGroup.acquire_buffer(buffer);
                if (ret == OK) {
                    memcpy((*buffer)->data(), packet.data, packet.size);
                    (*buffer)->set_range(0, packet.size);
                    (*buffer)->meta_data()->clear();
                    (*buffer)->meta_data()->setInt32(kKeyIsSyncFrame, packet.flags & AV_PKT_FLAG_KEY);
                    (*buffer)->meta_data()->setInt64(kKeyTime, packet.pts);
                }
                found = true;
            }
            av_free_packet(&packet);
        }

        return ret;
    }

protected:
    virtual ~CustomSource() {
        if (mConverter) {
            av_bitstream_filter_close(mConverter);
        }
        avformat_close_input(&mDataSource);
    }
private:
    AVFormatContext *mDataSource;
    AVCodecContext *mCodecContext;
    AVBitStreamFilterContext *mConverter;
    MediaBufferGroup mGroup;
    sp<MetaData> mFormat;
    int mVideoIndex;
};
*/


extern "C" {
//http://stackoverflow.com/questions/24675618/android-ffmpeg-bad-video-output
//Java_com_ror13_sysrazplayer_CFfmpeg_play( JNIEnv* env, jobject thiz, jstring path, jobject surface){





void
Java_com_ror13_sysrazplayer_CFfmpeg_play(JNIEnv *env, jobject thiz, jstring path, jobject surface){
    const char* filename = (env)->GetStringUTFChars( path, 0);
    ANativeWindow * nativeWindow = ANativeWindow_fromSurface(env,surface);

    JFFDemuxer demuxer(filename,false);
    JFFDecoder decoder(&demuxer);
    JFFVideoRender videoRender(decoder.getOutQueue(),nativeWindow);
    decoder.setNativeWindow(nativeWindow);
    demuxer.start();
    decoder.start();
    videoRender.start();
    for(;;)
        sleep(9999999);

/*
    const char* filename = (env)->GetStringUTFChars( path, 0);
__android_log_write(ANDROID_LOG_ERROR, "STAGEFIGHT", "1");
    class BLooper : public ALooper{};
    BLooper looper;
// At first, get an ANativeWindow from somewhere
    ANativeWindow * mNativeWindow = ANativeWindow_fromSurface(env,surface);
    ANativeWindow_setBuffersGeometry(mNativeWindow, 1920, 1080, WINDOW_FORMAT_RGBA_8888);
// Initialize the AVFormatSource from a video file
    MediaSource * mVideoSource = new CustomSource(filename);
    __android_log_write(ANDROID_LOG_ERROR, "STAGEFIGHT", "2");
// Once we get an MediaSource, we can encapsulate it with the OMXCodec now
    OMXClient mClient;
    mClient.connect();
    __android_log_write(ANDROID_LOG_ERROR, "STAGEFIGHT", "3");
    sp<MediaSource> mVideoDecoder = OMXCodec::Create(mClient.interface(), mVideoSource->getFormat(), false, mVideoSource);
    __android_log_write(ANDROID_LOG_ERROR, "STAGEFIGHT", "4");
    mVideoDecoder->start();
    __android_log_write(ANDROID_LOG_ERROR, "STAGEFIGHT", "5");
// Just loop to read decoded frames from mVideoDecoder
    for (;;) {
        MediaBuffer *mVideoBuffer;
        MediaSource::ReadOptions options;
        status_t err = mVideoDecoder->read(&mVideoBuffer, &options);
        if (err == OK) {
            if (mVideoBuffer->range_length() > 0) {
                // If video frame availabe, render it to mNativeWindow
                sp<MetaData> metaData = mVideoBuffer->meta_data();
                int64_t timeUs = 0;
                metaData->findInt64(kKeyTime, &timeUs);

                ANativeWindow_Buffer wbuffer;
                if(ANativeWindow_lock(mNativeWindow,&wbuffer,NULL) ==0 ) {
                    AVPicture picture;
                    memcpy(wbuffer.bits,mVideoBuffer->data(), mVideoBuffer->size());
                    ANativeWindow_unlockAndPost(mNativeWindow);
                }

            }
            mVideoBuffer->release();
        }
    }

// Finally release the resources
    //mVideoSource->clear();
    mVideoDecoder->stop();
    mVideoDecoder.clear();
    mClient.disconnect();
__android_log_write(ANDROID_LOG_ERROR, "STAGEFIGHT", "7");


    // Register all available file formats and codecs
    av_register_all();

    int err;

    // Open video file
    const char* filename = (env)->GetStringUTFChars( path, 0);
    //need enable fo release

    AVFormatContext* format_context = NULL;
    err = avformat_open_input(&format_context, filename, NULL, NULL);
    if (err < 0) {
        __android_log_write(ANDROID_LOG_ERROR, "ffmpeg", "Unable to open input file");
        return ;
    }

    // Retrieve stream information
    err = avformat_find_stream_info(format_context, NULL);
    if (err < 0) {
        __android_log_write(ANDROID_LOG_ERROR, "ffmpeg", "Unable to find stream info");
        return;
    }

    // Dump information about file onto standard error
    av_dump_format(format_context, 0, filename, 0);

    // Find the first video stream
    int video_stream;
    for (video_stream = 0; video_stream < format_context->nb_streams; ++video_stream) {
        if (format_context->streams[video_stream]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            break;
        }
    }
    if (video_stream == format_context->nb_streams) {
        __android_log_write(ANDROID_LOG_ERROR, "ffmpeg", "Unable to find video stream");
        return;
    }

    AVCodecContext* codec_context = format_context->streams[video_stream]->codec;
    AVCodec* codec = avcodec_find_decoder(codec_context->codec_id);
    err = avcodec_open2(codec_context, codec, NULL);
    if (err < 0) {
        __android_log_write(ANDROID_LOG_ERROR, "ffmpeg", "Unable to open codec");
        return;
    }



    struct SwsContext* img_convert_context;
    img_convert_context = sws_getCachedContext(NULL,
                                               codec_context->width, codec_context->height,
                                               codec_context->pix_fmt,
                                               codec_context->width, codec_context->height,
                                               PIX_FMT_RGBA  , SWS_BICUBIC,
                                               NULL, NULL, NULL);
    if (img_convert_context == NULL) {
        __android_log_write(ANDROID_LOG_ERROR, "ffmpeg", "Cannot initialize the conversion context");
        return ;
    }

    ANativeWindow* window = ANativeWindow_fromSurface(env,surface);
    ANativeWindow_setBuffersGeometry(window, codec_context->width, codec_context->height, WINDOW_FORMAT_RGBA_8888);

    AVFrame* frame = avcodec_alloc_frame();
    AVPacket packet;

    while (av_read_frame(format_context, &packet) >= 0) {
        if (packet.stream_index == video_stream) {
            // Video stream packet
            int frame_finished;
            avcodec_decode_video2(codec_context, frame, &frame_finished, &packet);

            if (frame_finished) {

                ANativeWindow_Buffer buffer;


                if(ANativeWindow_lock(window,&buffer,NULL) ==0 ) {
                    AVPicture picture;
                    avpicture_fill(&picture,(const uint8_t*)buffer.bits, AV_PIX_FMT_RGBA,buffer.stride,buffer.height);
                    sws_scale(img_convert_context,
                                frame->data, frame->linesize,
                                0, codec_context->height,
                                picture.data,
                                picture.linesize);

                    //__android_log_write(ANDROID_LOG_ERROR, "ffmpeg", msg);
                    ANativeWindow_unlockAndPost(window);
                }
            }
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);

    }

    sws_freeContext(img_convert_context);

    // Free the YUV frame
    av_free(frame);

    // Close the codec
    avcodec_close(codec_context);

    // Close the video file
    avformat_close_input(&format_context);

    ANativeWindow_release(window);
    (env)->ReleaseStringUTFChars(path, filename);
*/
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