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



void CPlayer::open(ANativeWindow* nativeWindow, CplayerConfig * conf){
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

    audioDecoder = new CAudioDecoder(demuxer);
    audioRender = new CAudioRender(audioDecoder->getOutQueue());


}
void CPlayer::close(){
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
    if(audioDecoder != NULL){
        delete audioDecoder;
    }
    if(audioRender != NULL){
        delete audioRender;
    }
};

void CPlayer::start(){
    demuxer->start();
    decoder->start();
    videoRender->start();
    audioDecoder->start();
    audioRender->start();

    mIsPlaying = true;
}
void CPlayer::stop(){
    videoRender->stop();
    decoder->stop();
    demuxer->stop();
    audioDecoder->stop();
    audioRender->stop();
    mIsPlaying = false;
}


CPlayer::CPlayer(){
    demuxer = NULL;
    decoder = NULL;
    videoRender = NULL;
    mIsPlaying = false;
}

CPlayer::~CPlayer(){
    close();
};
