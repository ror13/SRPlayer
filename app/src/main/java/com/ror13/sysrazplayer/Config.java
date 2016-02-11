package com.ror13.sysrazplayer;

import android.content.Context;
import android.util.Log;
import android.widget.EditText;

import java.util.HashMap;

/**
 * Created by ror13 on 1/27/16.
 */
public class Config extends HashMap<Object,Object> {
    public static final int OPT_URI = 0;
    public static final int OPT_RTSP_PROTOCOL = 1;
    public static final int OPT_PACKET_BUFFER_SIZE = 2;
    public static final int OPT_IS_FLUSH = 3;
    public static final int OPT_IS_MAX_FPS = 4;
    public static final int OPT_IS_SKIP_PACKET = 5;
    public static final int OPT_IS_LOOP_PLAYING = 6;
    public static final int OPT_IS_WINDOW_NATIVE= 7;
    public static final int OPT_IS_WINDOW_GLES= 8;
    private static Config singleton;
    protected Config(){

        //et.setText("rtsp://10.10.25.253:554/hdmi");
        //String uri ="/storage/sdcard0/Download/startrekintodarkness-usajj60sneak_h1080p.mov";
        String uri ="/storage/sdcard0/Download/bbb_sunflower_1080p_30fps_normal.mp4";
        //String uri = "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov";

        put(OPT_URI,uri);
        put(OPT_RTSP_PROTOCOL,"tcp");
        put(OPT_PACKET_BUFFER_SIZE,10);
        put(OPT_IS_FLUSH,false);
        put(OPT_IS_MAX_FPS,true);
        put(OPT_IS_SKIP_PACKET, false);
        put(OPT_IS_LOOP_PLAYING, true);
        put(OPT_IS_WINDOW_NATIVE, false);
        put(OPT_IS_WINDOW_GLES, true);

    }

    public int getValInt(int opt){ return (Integer) get(opt);}
    public boolean getValBool(int opt){ return (Boolean) get(opt);}
    public Object getValObj(int opt){ return get(opt);}



    public static Config getInstance(){
        if (singleton == null){
            singleton = new Config();
        }
        return singleton;
    }

}
