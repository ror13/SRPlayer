package com.ror13.sysrazplayer;

import android.content.Context;
import android.widget.EditText;

/**
 * Created by ror13 on 1/27/16.
 */
public class Config {
    private static Config singleton;
    protected Config(){
        mRtspProtocol = "udp";
        mPacketBufferSize = 10;
        mIsFlush = true;
        mIsMaxFps = true;
        mIsSkipPacket = true;
        //et.setText("rtsp://10.10.25.253:554/hdmi");
        mUri ="/storage/sdcard0/Download/startrekintodarkness-usajj60sneak_h1080p.mov";
        //et.setText("rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov");

    }

    public static Config getInstance(){
        if (singleton == null){
            singleton = new Config();
        }
        return singleton;
    }

    String mUri;
    String mRtspProtocol;
    int mPacketBufferSize;
    boolean mIsFlush;
    boolean mIsMaxFps;
    boolean mIsSkipPacket;

}
