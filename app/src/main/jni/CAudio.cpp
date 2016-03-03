//
// Created by ror131 on 3/1/16.
//

#include "CAudio.h"

#define AUDIO_OUT_DEVICE_CHANELS 2
#define AUDIO_DEVICE_SAMPLERATE 44100


CAudioDecoder::CAudioDecoder(CQueue <CMessage>* inputQueue):CThread(queueAudioDecoding) {
    mInputQueue = inputQueue;
    mSwrContext = NULL;
    mCodecContext = NULL;
}

void CAudioDecoder::configure(AVCodecContext* codecContext){
    mCodecContext = codecContext;
    AVCodec *codec = avcodec_find_decoder(mCodecContext->codec_id);;
    avcodec_open2(mCodecContext, codec, NULL);

    //mSwrContext = swr_alloc();
    mSwrContext = swr_alloc_set_opts(NULL,
                           av_get_default_channel_layout(AUDIO_OUT_DEVICE_CHANELS),
                           AV_SAMPLE_FMT_S16, // defaul for output device
                           AUDIO_DEVICE_SAMPLERATE,
                           mCodecContext->channel_layout,
                           mCodecContext->sample_fmt ,
                           mCodecContext->sample_rate,
                           0,
                           NULL);
    swr_init(mSwrContext);

}

void CAudioDecoder::clear() {
    if(mSwrContext && swr_is_initialized(mSwrContext)){
        swr_close(mSwrContext);
    }

    if(mSwrContext){
        swr_free(&mSwrContext);
        mSwrContext = NULL;
    }
    if(mCodecContext){
        avcodec_close(mCodecContext);
        avcodec_free_context(&mCodecContext);
        mCodecContext = NULL;
    }


}



void * CAudioDecoder::queueAudioDecoding(void * baseObj){
    CAudioDecoder * self = (CAudioDecoder*) baseObj;

    for(;;){
        AVPacket * packet = NULL;

        if(self->isCancel()){
            break;
        }
        self->mInputQueue->acquire();
        if (self->mInputQueue->empty()) {
            self->mInputQueue->release();
            continue;
        }

        MessageType msgType = self->mInputQueue->front().type;
        void * msgData = self->mInputQueue->front().data;
        self->mInputQueue->pop_front();
        self->mInputQueue->release();

        if(msgType == MessageType::AUDIO_CODEC_CONFIG){
            self->clear();
            self->configure((AVCodecContext*)msgData);
            continue;
        }

        packet = (AVPacket*) msgData;
        if(!packet || !self->mSwrContext){
            continue;
        }

        CAudioFrame* outFrame = new CAudioFrame;
        AVFrame  pDecodedFrame = {0};
        int nGotFrame = 0;

        int ret = avcodec_decode_audio4(self->mCodecContext,
                                        &pDecodedFrame,
                                        &nGotFrame,
                                        packet);
        if(ret < 0) LOG_ERROR("avcodec_decode_audio4  %d", ret);

        int out_linesize = 0;
        int needed_buf_size = av_samples_get_buffer_size(&out_linesize, AUDIO_OUT_DEVICE_CHANELS,
                                                         pDecodedFrame.nb_samples, AV_SAMPLE_FMT_S16, 1);

        outFrame->data = new uint8_t[needed_buf_size];
        uint8_t *out[] = { (uint8_t *)outFrame->data };
        int outsamples = swr_convert(self->mSwrContext, out, out_linesize, (const uint8_t**)pDecodedFrame.data, pDecodedFrame.nb_samples);

        outFrame->size = outsamples * AUDIO_OUT_DEVICE_CHANELS * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
        if(outsamples < 0){
            char errbuf[256];
            LOG_ERROR("avresample_convert_frame  %s", av_make_error_string (errbuf, 256,  outsamples));
        }

        av_frame_unref(&pDecodedFrame);

        clearCMessage(msgType, msgData);

        self->mOutQueue.acquire();
        self->mOutQueue.push_back(outFrame);
        self->mOutQueue.release();

    }


    self->clear();


    self->flush();
}

void CAudioDecoder::flush() {
    mOutQueue.acquire();
    while(!mOutQueue.empty()){
        CAudioFrame* audioFrame = (CAudioFrame*) mOutQueue.front();
        mOutQueue.pop_front();
        if(audioFrame && audioFrame->data){
            delete audioFrame->data;
        }
        if(audioFrame){
            delete audioFrame;
        }
    }
    mOutQueue.release();
}

//====================================================================


CAudioRender::~CAudioRender()
{

   clear();


    if (mOutputMixObj)
        (*mOutputMixObj)->Destroy(mOutputMixObj);
    if (mEngineObj)
        (*mEngineObj)->Destroy(mEngineObj);

    mOutputMixObj = NULL;
    mEngineObj = NULL;

}

void CAudioRender::clear()
{
    if (mPlayerObject)
        (*mPlayerObject)->Destroy(mPlayerObject);
    if(mCurrentPlayingBuff){
        delete mCurrentPlayingBuff;
    }
    mPlayerObject = NULL;
    mCurrentPlayingBuff = NULL;
}

CAudioRender::CAudioRender(CQueue <CAudioFrame*>* inputQueue):CThread(queueAudioRender)
{
    mCurrentPlayingBuff = NULL;
    mInputQueue = inputQueue;
    const SLInterfaceID pIDs[1] = {SL_IID_ENGINE};
    const SLboolean pIDsRequired[1]  = {SL_BOOLEAN_TRUE};
    const SLInterfaceID pOutputMixIDs[] = {};
    const SLboolean pOutputMixRequired[] = {};

    SLresult result = slCreateEngine(&mEngineObj, 0, NULL, 1, pIDs, pIDsRequired);
    if(result) LOG_ERROR("Error after slCreateEngine");

    result = (*mEngineObj)->Realize(mEngineObj, SL_BOOLEAN_FALSE);
    if(result) LOG_ERROR("Error after Realize engineObj");

    result = (*mEngineObj)->GetInterface(mEngineObj, SL_IID_ENGINE, &mEngine);
    if(result) LOG_ERROR("(*mEngineObj)->GetInterface(  %d", result);

    result = (*mEngine)->CreateOutputMix(mEngine, &mOutputMixObj, 0, pOutputMixIDs, pOutputMixRequired);
    if(result) LOG_ERROR("CreateOutputMix  %d", result);

    result = (*mOutputMixObj)->Realize(mOutputMixObj, SL_BOOLEAN_FALSE);
    if(result) LOG_ERROR("mOutputMixObj)->Realize  %d", result);
}

void CAudioRender::configure() {
    DEBUG_PRINT_LINE;
    SLDataLocator_AndroidSimpleBufferQueue locatorBufferQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1}; /*one budder in queueи*/
    SLDataFormat_PCM formatPCM = {
            SL_DATAFORMAT_PCM, AUDIO_OUT_DEVICE_CHANELS, AUDIO_DEVICE_SAMPLERATE*1000,
            SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN
    };
    SLDataSource audioSrc = {&locatorBufferQueue, &formatPCM};
    SLDataLocator_OutputMix locatorOutMix = {SL_DATALOCATOR_OUTPUTMIX, mOutputMixObj};
    SLDataSink audioSnk = {&locatorOutMix, NULL};
    const SLInterfaceID pIDs[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean pIDsRequired[1] = {SL_BOOLEAN_TRUE };

    SLresult result = (*mEngine)->CreateAudioPlayer(mEngine, &mPlayerObject, &audioSrc, &audioSnk, 1, pIDs, pIDsRequired);
    if(result) LOG_ERROR("(*mEngine)->CreateAudioPlayer err: %d", result);

    result = (*mPlayerObject)->Realize(mPlayerObject, SL_BOOLEAN_FALSE);
    if(result) LOG_ERROR("(*mPlayerObject)->Realize err: %d", result);

    result = (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_PLAY, &mPlayer);
    if(result) LOG_ERROR("(*mPlayerObject)->GetInterface SL_IID_PLAY err: %d", result);

    result = (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_BUFFERQUEUE, &mBufferQueue);
    if(result) LOG_ERROR("((*mPlayerObject)->GetInterface SL_IID_BUFFERQUEUE err: %d", result);

    result = (*mBufferQueue)->RegisterCallback(mBufferQueue, bufferQueuePlayerCallBack, this);
    if(result) LOG_ERROR("(*mBufferQueue)->RegisterCallback err: %d", result);

    result = (*mPlayer)->SetPlayState(mPlayer, SL_PLAYSTATE_PLAYING);
    if(result) LOG_ERROR("(*mPlayer)->SetPlayState SL_PLAYSTATE_PLAYING err: %d", result);

}

//==============================

void CAudioRender::bufferQueuePlayerCallBack (SLBufferQueueItf bufferQueue, void *baseObj) {
    CAudioRender* self = (CAudioRender*) baseObj;

    CAudioFrame* audioFrame = NULL;
    for(;;) {
        if(self->isCancel()){
            return;
        }
        self->mInputQueue->acquire();
        if (!self->mInputQueue->empty()) {
            audioFrame = self->mInputQueue->front();
            self->mInputQueue->pop_front();
        }
        self->mInputQueue->release();
        if (audioFrame) {
            break;
        }
    }

    (*self->mBufferQueue)->Enqueue(self->mBufferQueue, audioFrame->data, audioFrame->size);

    /* need save sound buffer for end playing */
    if(self->mCurrentPlayingBuff){
        delete self->mCurrentPlayingBuff;
    }
    self->mCurrentPlayingBuff = audioFrame->data;
    delete  audioFrame;
}


void * CAudioRender::queueAudioRender(void * baseObj){
    CAudioRender* self = (CAudioRender*) baseObj;
    self->configure();

    (*self->mBufferQueue)->Clear(self->mBufferQueue);
    /* need for start playing*/
    self->bufferQueuePlayerCallBack(self->mBufferQueue,self); // no waite

    for(;;){
        sleep(1);
        if(self->isCancel()){
            (*self->mPlayer)->SetPlayState(self->mPlayer, SL_PLAYSTATE_STOPPED);
            break;
        }
    }

    self->clear();
    return NULL;
}
