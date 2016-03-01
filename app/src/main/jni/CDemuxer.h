//
// Created by ror131 on 3/1/16.
//

#ifndef SYSRAZPLAYER_CDEMUXER_H
#define SYSRAZPLAYER_CDEMUXER_H

extern "C" {
#include <libavformat/avformat.h>
}

#include "utils.h"

class CDemuxer : public CThread {
public:
    CDemuxer();
    ~CDemuxer();
    void openFile(const char * path);
    void closeFile();
    const AVStream* getVideoStream(){return mVideoStream != -1 ? mFormatContext->streams[mVideoStream] : NULL;}
    const AVStream* getAudioStream(){return mAudioStream != -1 ? mFormatContext->streams[mAudioStream] : NULL;}
    CQueue <CMessage>* getVideoQueue(){return &mVideoQueue;};
    CQueue <CMessage>* getAudioQueue(){return &mAudioQueue;};
    void configure(bool isLoop = false, bool isSkipPacket = false, unsigned int packetBufferSize = 10, const char * rtspProtocolType = "tcp");
    void setFlushOnOpen(bool flushOnOpen){mFlushOnOpen = flushOnOpen;}
    void flush();
    bool isEof(){ return mEof;};

protected:
    static void * fileReading(void * baseObj);

    AVFormatContext* mFormatContext;
    int mVideoStream,
            mAudioStream;
    CQueue <CMessage> mVideoQueue,
            mAudioQueue;
    pthread_t mThread;
    bool mEof;
    bool mIsSkipPacket;
    unsigned int mPacketBufferSize;
    std::string  mRtspProtocolType;
    bool mFlushOnOpen;
    bool mIsLoop;
};
#endif //SYSRAZPLAYER_CDEMUXER_H
