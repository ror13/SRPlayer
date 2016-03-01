//
// Created by ror131 on 3/1/16.
//

#include "CVideo.h"

#include <binder/ProcessState.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <media/stagefright/ColorConverter.h>
#include <media/stagefright/foundation/ALooper.h>
#include <utils/List.h>

using namespace android;

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




//=========================================================


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
        AVCodecContext* codecContext = NULL;

        for (;;) {
            mInputQueu->acquire();
            if (!mInputQueu->empty()) {
                if(mInputQueu->front().type == VIDEO_CODEC_CONFIG){
                    codecContext = (AVCodecContext*) mInputQueu->front().data;
                    mInputQueu->pop_front();
                }
                break;
            }
            mInputQueu->release();

        }

        AVPacket * packet = (AVPacket *)mInputQueu->front().data;
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
    CQueue <CMessage>* mInputQueu;
    CDemuxer *mDemuxer;

    MediaBufferGroup mGroup;
    sp<MetaData> mFormat;

    AVBitStreamFilterContext* mConverter;

};

//========================================================================

CDecoder::CDecoder(CDemuxer * demuxer):CThread(queueVideoDecoding) {
    mDemuxer = demuxer;
    mNativeWindow = NULL;
    mIsFlushDemuxer = false;
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
        if (err != OK || videoBuffer->range_length() <= 0) {
            continue;
        }


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

    self->flush();
    videoDecoder->stop();
    client.disconnect();

    return NULL;

}
