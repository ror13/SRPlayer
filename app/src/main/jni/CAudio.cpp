//
// Created by ror131 on 3/1/16.
//

#include "CAudio.h"

#define AUDIO_DEVICE_CHANELS 2


CAudioDecoder::CAudioDecoder(CDemuxer * demuxer):CThread(queueAudioDecoding) {
    mDemuxer = demuxer;
}

void * CAudioDecoder::queueAudioDecoding(void * baseObj){
    CAudioDecoder * self = (CAudioDecoder*) baseObj;

    const AVStream* audioCodec = self->mDemuxer->getAudioStream();
    if(audioCodec <= 0){
        return NULL;
    }

    AVCodec *codec = avcodec_find_decoder(audioCodec->codec->codec_id);;
    avcodec_open2(audioCodec->codec, codec, NULL);

    SwrContext *swr = swr_alloc();
    swr=swr_alloc_set_opts(NULL,
                           AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100, // output params
                           audioCodec->codec->channel_layout,
                           audioCodec->codec->sample_fmt ,
                           audioCodec->codec->sample_rate,
                           0,
                           NULL);
    swr_init(swr);

    CQueue <CMessage> * inputQueu = self->mDemuxer->getAudioQueue();

    for(;;){
        AVPacket * packet = NULL;

        if(self->isCancel()){
            break;
        }
        inputQueu->acquire();
        if (!inputQueu->empty()) {
            if(inputQueu->front().type == AUDIO_PKT)
                packet = (AVPacket*) inputQueu->front().data;
            inputQueu->pop_front();
        }
        inputQueu->release();

        if(!packet){
            continue;
        }

        CAudioFrame* outFrame = new CAudioFrame;
        AVFrame  pDecodedFrame = {0};
        int nGotFrame = 0;

        int ret = avcodec_decode_audio4(audioCodec->codec,
                                        &pDecodedFrame,
                                        &nGotFrame,
                                        packet);
        if(ret < 0) LOG_ERROR("avcodec_decode_audio4  %d", ret);

        int out_linesize = 0;
        int needed_buf_size = av_samples_get_buffer_size(&out_linesize, AUDIO_DEVICE_CHANELS,
                                                         pDecodedFrame.nb_samples, AV_SAMPLE_FMT_S16, 1);

        outFrame->data = new uint8_t[needed_buf_size];
        uint8_t *out[] = { (uint8_t *)outFrame->data };
        int outsamples = swr_convert(swr, out, out_linesize, (const uint8_t**)pDecodedFrame.data, pDecodedFrame.nb_samples);

        outFrame->size = outsamples * 2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
        if(outsamples < 0){
            char errbuf[256];
            LOG_ERROR("avresample_convert_frame  %s", av_make_error_string (errbuf, 256,  outsamples));
        }

        av_frame_unref(&pDecodedFrame);
        av_free_packet(packet);
        delete packet;

        self->mOutQueue.acquire();
        self->mOutQueue.push_back(outFrame);
        self->mOutQueue.release();

    }

    if(swr && swr_is_initialized(swr)){
        swr_close(swr);
    }
    if(swr){
        swr_free(&swr);
    }

    //avcodec_close(codec);

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
            SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
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
