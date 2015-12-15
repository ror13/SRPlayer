//
// Created by ror13 on 13.12.15.
//

#include "CFfmpeg.h"

#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>


//http://stackoverflow.com/questions/24675618/android-ffmpeg-bad-video-output
 Java_com_ror13_sysrazplayer_CFfmpeg_play( JNIEnv* env, jobject thiz, jstring path, jobject surface){


    // Register all available file formats and codecs
    av_register_all();

    int err;

    // Open video file
    const char* filename = (*env)->GetStringUTFChars(env, path, 0);
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
                    avpicture_fill(&picture,buffer.bits, AV_PIX_FMT_RGBA,buffer.stride,buffer.height);
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
    (*env)->ReleaseStringUTFChars(env, path, filename);

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

    return (*env)->NewStringUTF(env, "Hello from JNI !  Compiled with ABI " ABI ".");
}