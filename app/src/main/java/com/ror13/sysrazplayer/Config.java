package com.ror13.sysrazplayer;

import android.app.Activity;
import android.content.Context;
import android.util.Log;
import android.widget.EditText;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
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
    public static final int OPT_IS_WINDOW_GLES = 8;
    public static final int OPT_IS_VIDEO_QUEUE = 9;
    private static Config singleton;
    private SharedPreferences sPref;
    protected Config(){

        Context appContext = SysRazApp.getInstance().getBaseContext();
        sPref = appContext.getSharedPreferences("Config", appContext.MODE_PRIVATE);





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


    void save() {
        Editor ed = sPref.edit();

        ed.putString("OPT_URI", getValObj(OPT_URI).toString());
        ed.putString("OPT_RTSP_PROTOCOL", getValObj(OPT_RTSP_PROTOCOL).toString());
        ed.putInt("OPT_PACKET_BUFFER_SIZE", getValInt(OPT_PACKET_BUFFER_SIZE));
        ed.putBoolean("OPT_IS_FLUSH", getValBool(OPT_IS_FLUSH));
        ed.putBoolean("OPT_IS_MAX_FPS", getValBool(OPT_IS_MAX_FPS));
        ed.putBoolean("OPT_IS_SKIP_PACKET", getValBool(OPT_IS_SKIP_PACKET));
        ed.putBoolean("OPT_IS_LOOP_PLAYING", getValBool(OPT_IS_LOOP_PLAYING));
        ed.putBoolean("OPT_IS_WINDOW_NATIVE", getValBool(OPT_IS_WINDOW_NATIVE));
        ed.putBoolean("OPT_IS_WINDOW_GLES", getValBool(OPT_IS_WINDOW_GLES));
        ed.putBoolean("OPT_IS_VIDEO_QUEUE", getValBool(OPT_IS_VIDEO_QUEUE));
        ed.commit();
    }

    void load() {
        //et.setText("rtsp://10.10.25.253:554/hdmi");
        //String uri ="/storage/sdcard0/Download/startrekintodarkness-usajj60sneak_h1080p.mov";
        String uri ="/storage/sdcard0/Download/bbb_sunflower_1080p_30fps_normal.mp4";
        //String uri = "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov";
        put(OPT_URI, sPref.getString("OPT_URI", uri));
        put(OPT_RTSP_PROTOCOL,sPref.getString("OPT_RTSP_PROTOCOL", "tcp"));
        put(OPT_PACKET_BUFFER_SIZE,sPref.getInt("OPT_PACKET_BUFFER_SIZE", 10));
        put(OPT_IS_FLUSH,sPref.getBoolean("OPT_IS_FLUSH", true));
        put(OPT_IS_MAX_FPS,sPref.getBoolean("OPT_IS_MAX_FPS", true));
        put(OPT_IS_SKIP_PACKET, sPref.getBoolean("OPT_IS_SKIP_PACKET", true));
        put(OPT_IS_LOOP_PLAYING, sPref.getBoolean("OPT_IS_LOOP_PLAYING", true));
        put(OPT_IS_WINDOW_NATIVE, sPref.getBoolean("OPT_IS_WINDOW_NATIVE", false));
        put(OPT_IS_WINDOW_GLES, sPref.getBoolean("OPT_IS_WINDOW_GLES", true));
        put(OPT_IS_VIDEO_QUEUE, sPref.getBoolean("OPT_IS_VIDEO_QUEUE", false));
    }

}
