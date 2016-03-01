//
// Created by ror13 on 13.12.15.
//

#ifndef SRPLAYER_CFFMPEG_H
#define SRPLAYER_CFFMPEG_H
#include "utils.h"
#include "CDemuxer.h"
#include "CAudio.h"
#include "CVideo.h"

class CPlayer {
public:
    typedef struct {
        std::string uri;
        std::string rtspProtocol;
        int packetBufferSize;
        bool isFlush;
        bool isMaxFps;
        bool isSkipPacket;
        bool isLoopPlaying;
        bool isWindowNative;
        bool isWindowGles;
        bool isVideoQueue;

    } CplayerConfig;
    void open(ANativeWindow* nativeWindow, CplayerConfig * conf);
    void close();
    void start();
    void stop();
    CPlayer();
    ~CPlayer();
private:
    CDemuxer* demuxer;
    CDecoder* decoder;
    CAudioRender* audioRender;
    CAudioDecoder* audioDecoder;
    CVideoRender* videoRender;
    bool mIsPlaying;
};

#endif //SRPLAYER_CFFMPEG_H
