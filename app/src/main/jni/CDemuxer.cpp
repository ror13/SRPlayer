//
// Created by ror131 on 3/1/16.
//

#include "CDemuxer.h"


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
    mEof = false;
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


    mVideoQueue.acquire();
    if(mVideoStream != -1 && mFormatContext->streams[mVideoStream]){
        AVCodecContext* context = avcodec_alloc_context3(NULL);
        avcodec_copy_context(context, mFormatContext->streams[mVideoStream]->codec);
        mVideoQueue.push_back({MessageType::VIDEO_CODEC_CONFIG,context});
    }
    mVideoQueue.release();

    mAudioQueue.acquire();
    if(mAudioStream != -1 && mFormatContext->streams[mAudioStream]){
        AVCodecContext* context = avcodec_alloc_context3(NULL);
        avcodec_copy_context(context, mFormatContext->streams[mAudioStream]->codec);
        mAudioQueue.push_back({MessageType::AUDIO_CODEC_CONFIG,context});
    }
    mAudioQueue.release();

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
        clearCMessage(&mVideoQueue.front());
        mVideoQueue.pop_front();
    }
    mVideoQueue.release();

    mAudioQueue.acquire();
    while(!mAudioQueue.empty()){
        clearCMessage(&mAudioQueue.front());
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
                        clearCMessage(&self->mVideoQueue.back());
                        self->mVideoQueue.pop_back();
                    }
                    if(!self->mAudioQueue.empty()){
                        clearCMessage(&self->mAudioQueue.back());
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
            self->mAudioQueue.push_back({MessageType::AUDIO_PKT,packet});
            self->mAudioQueue.release();
            continue;
        }
        if (packet->stream_index == self->mVideoStream) {
            self->mVideoQueue.acquire();
            self->mVideoQueue.push_back({MessageType::VIDEO_PKT,packet});
            self->mVideoQueue.release();
            continue;
        }
    }
    return NULL;
}
