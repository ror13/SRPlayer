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
#include <utils/List.h>
#include <new>
#include <map>

extern "C" {
#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

using namespace android;





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
        mVideoTrack = mDataSource->streams[mVideoIndex]->codec;

        size_t bufferSize = (mVideoTrack->width * mVideoTrack->height * 3) / 2;
        mGroup.add_buffer(new MediaBuffer(bufferSize));
        mFormat = new MetaData;

        switch (mVideoTrack->codec_id) {
            case CODEC_ID_H264:
                mConverter = av_bitstream_filter_init("h264_mp4toannexb");
                mFormat->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);
                if (mVideoTrack->extradata[0] == 1) {
                    mFormat->setData(kKeyAVCC, kTypeAVCC, mVideoTrack->extradata, mVideoTrack->extradata_size);
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

        mFormat->setInt32(kKeyWidth, mVideoTrack->width);
        mFormat->setInt32(kKeyHeight, mVideoTrack->height);
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
        AVPacket packet;
        status_t ret;
        bool found = false;

        while (!found) {
            ret = av_read_frame(mDataSource, &packet);
            if (ret < 0) {
                return ERROR_END_OF_STREAM;
            }

            if (packet.stream_index == mVideoIndex) {
                if (mConverter) {
                    av_bitstream_filter_filter(mConverter, mVideoTrack, NULL, &packet.data, &packet.size, packet.data, packet.size, packet.flags & AV_PKT_FLAG_KEY);
                }
                ret = mGroup.acquire_buffer(buffer, true);
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
    AVCodecContext *mVideoTrack;
    AVBitStreamFilterContext *mConverter;
    MediaBufferGroup mGroup;
    sp<MetaData> mFormat;
    int mVideoIndex;
};



extern "C" {
//http://stackoverflow.com/questions/24675618/android-ffmpeg-bad-video-output
//Java_com_ror13_sysrazplayer_CFfmpeg_play( JNIEnv* env, jobject thiz, jstring path, jobject surface){





void
Java_com_ror13_sysrazplayer_CFfmpeg_play(JNIEnv *env, jobject thiz, jstring path, jobject surface){

    const char* filename = (env)->GetStringUTFChars( path, 0);
__android_log_write(ANDROID_LOG_ERROR, "STAGEFIGHT", "1");

// At first, get an ANativeWindow from somewhere
    sp<ANativeWindow> mNativeWindow = ANativeWindow_fromSurface(env,surface);

// Initialize the AVFormatSource from a video file
    MediaSource * mVideoSource = new CustomSource(filename);

// Once we get an MediaSource, we can encapsulate it with the OMXCodec now
    OMXClient mClient;
    mClient.connect();
    sp<MediaSource> mVideoDecoder = OMXCodec::Create(mClient.interface(), mVideoSource->getFormat(), false, mVideoSource, NULL, 0, mNativeWindow);
    mVideoDecoder->start();

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
                native_window_set_buffers_timestamp(mNativeWindow.get(), timeUs * 1000);
                err = mNativeWindow->queueBuffer(mNativeWindow.get(), mVideoBuffer->graphicBuffer().get(),-1);
                if (err == 0) {
                    metaData->setInt32(kKeyRendered, 1);
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
    //const char* filename = (env)->GetStringUTFChars( path, 0);
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