//
// Created by ror131 on 3/1/16.
//

#include "CVideo.h"
#include "utils.h"

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

CVideoRender::CVideoRender(CQueue <CMessage>* inputQueue, ANativeWindow * nativeWindow) :CThread(queueVideoRendering){
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
    //videoBuffer->release();
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

        self->mInputQueue->acquire();
        if(self->mInputQueue->empty()) {
            self->mInputQueue->release();
            continue;
        }
        MessageType msgType = self->mInputQueue->front().type;
        void * msgData = self->mInputQueue->front().data;

        self->mInputQueue->pop_front();
        self->mInputQueue->release();

        /*
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
*/
        switch(msgType){
            case MessageType::VIDEO_RENDER_CONFIG:{
                CVideoFrameConfig* frameConfig = (CVideoFrameConfig*) msgData;
                self->initializeRender(frameConfig->colorspace, frameConfig->width, frameConfig->height);
                break;
            }
            case MessageType::VIDEO_FRAME:{
                CVideoFrame* frame = (CVideoFrame*)msgData;
                self->drawFrame(frame->data);
                break;
            }
        }

        //__android_log_print(ANDROID_LOG_INFO, "FPS", " %ld", GFps_GetCurFps());

        clearCMessage(msgType,msgData);


    }
    self->desroyRender();
    return NULL;
}




//=========================================================


class CVideoSource : public MediaSource {
public:
    CVideoSource(AVCodecContext* codecContext, CQueue <CMessage>* inputQueu) {
        mConverter = NULL;
        mCodecContext = codecContext;
        mInputQueu = inputQueu;
        mFormat = new MetaData;
        size_t bufferSize = (mCodecContext->width * mCodecContext->height * 3) / 2;
        mGroup.add_buffer(new MediaBuffer(bufferSize));

        switch (mCodecContext->codec_id) {
            case CODEC_ID_H264:
                mFormat->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);
                mConverter = av_bitstream_filter_init("h264_mp4toannexb");
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
        AVPacket* packet= NULL;

        for (;;) {
            mInputQueu->acquire();
            if (!mInputQueu->empty()) {
                if(mInputQueu->front().type == MessageType::VIDEO_PKT){
                    packet = (AVPacket*) mInputQueu->front().data;
                    mInputQueu->pop_front();
                    mInputQueu->release();
                    break;
                }
                //mInputQueu->release();
                return ERROR_END_OF_STREAM;
            }
            mInputQueu->release();
        }

        ret = mGroup.acquire_buffer(buffer);

        if(packet == NULL){
            return  ret;
        }

        AVPacket packetOut= *packet;
        if (mConverter) {
            av_bitstream_filter_filter(mConverter, mCodecContext, NULL, &packetOut.data, &packetOut.size, packet->data, packet->size, packet->flags & AV_PKT_FLAG_KEY);
        }
        //__android_log_print(ANDROID_LOG_INFO, "sometag", "paket size %d", packet->size);

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

        av_free_packet((AVPacket*)packet);
        return ret;
    }


    virtual ~CVideoSource() {
        if (mConverter) {
            av_bitstream_filter_close(mConverter);
        }
        mFormat = NULL;
        avcodec_free_context(&mCodecContext);
    }
private:
    CQueue <CMessage>* mInputQueu;

    MediaBufferGroup mGroup;
    sp<MetaData> mFormat;

    AVCodecContext* mCodecContext;
    AVBitStreamFilterContext* mConverter;

};

//========================================================================

CDecoder::CDecoder(CQueue <CMessage>* inputQueue):CThread(queueVideoDecoding) {
    mInputQueue = inputQueue;
    mNativeWindow = NULL;
    mIsFlushDemuxer = false;
}

void CDecoder::flush() {
    mOutQueue.acquire();
    while(!mOutQueue.empty()){
        clearCMessage(&mOutQueue.front());
        mOutQueue.pop_front();
    }
    mOutQueue.release();
}




void * CDecoder::queueVideoDecoding(void * baseObj){
    CDecoder * self = (CDecoder*) baseObj;
    OMXClient client;
    client.connect();
    sp<MediaSource> videoSource = NULL;
    sp<MediaSource> videoDecoder = NULL;

    //if(self->mIsFlushDemuxer){
    //    self->mDemuxer->flush();
    //}


    for (;;) {
        MessageType msgType = MessageType::EMPTY_MESSAGE;
        void * msgData = NULL;

        if(self->isCancel()){
            break;
        }

        self->mInputQueue->acquire();
        if (self->mInputQueue->empty()) {
            self->mInputQueue->release();
            continue;
        }

        msgType = self->mInputQueue->front().type;
        // pkt reading from video source
        if(msgType != VIDEO_PKT){
            msgData = self->mInputQueue->front().data;
            self->mInputQueue->pop_front();
        }
        self->mInputQueue->release();

        if(msgType == VIDEO_CODEC_CONFIG){
            if(videoDecoder.get()){
                videoDecoder->stop();
            }

            videoSource = new CVideoSource((AVCodecContext*)msgData, self->mInputQueue);
            videoDecoder = OMXCodec::Create(client.interface(), videoSource->getFormat(), \
               false, videoSource, NULL, OMXCodec::kHardwareCodecsOnly | OMXCodec::kOnlySubmitOneInputBufferAtOneTime,
                                            self->mNativeWindow);
            videoDecoder->start();

            CVideoFrameConfig * frameConfig = new CVideoFrameConfig();
            videoSource->getFormat()->findInt32(kKeyWidth, &frameConfig->width);
            videoSource->getFormat()->findInt32(kKeyHeight, &frameConfig->height);
            videoDecoder->getFormat()->findInt32(kKeyColorFormat, &frameConfig->colorspace);

            self->mOutQueue.acquire();
            self->mOutQueue.push_back({MessageType::VIDEO_RENDER_CONFIG,frameConfig});
            self->mOutQueue.release();
        }

        if(msgType == VIDEO_PKT && videoDecoder.get() && videoSource.get()){

            MediaBuffer* videoBuffer;
            MediaSource::ReadOptions options;
            status_t err = videoDecoder->read(&videoBuffer, &options);
            if (err != OK || videoBuffer->range_length() <= 0) {
                continue;
            }

            CVideoFrame * frame = new CVideoFrame();
            frame->data = videoBuffer;
            frame->pts = 0;

            self->mOutQueue.acquire();
            self->mOutQueue.push_back({MessageType::VIDEO_FRAME,frame});
            self->mOutQueue.release();
        }
        // not need clear, becouse clear in another objects
        //clearCMessage(msgType, msgData);
    }

    self->flush();
    if(videoDecoder.get()){
        videoDecoder->stop();
    }
    client.disconnect();

    return NULL;

}
