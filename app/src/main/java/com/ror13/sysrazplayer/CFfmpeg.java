package com.ror13.sysrazplayer;

import android.os.Build;
import android.util.Log;
import android.view.Surface;

/**
 * Created by ror13 on 13.12.15.
 */
public class CFfmpeg extends Thread{
    String path;
    Surface surface;

    public void setPath(String path){
        this.path =path;
    }
    public void setSurface(Surface surface){
        this.surface =surface;
    }
    static {
        int currentapiVersion = android.os.Build.VERSION.SDK_INT;
        if (currentapiVersion > Build.VERSION_CODES.KITKAT){
            System.loadLibrary("CFfmpeg_v5");
        } else{
            System.loadLibrary("CFfmpeg_v4");
        }

    }
    public void run() {
        play(path, surface);
    }
    public native String tst();
    public native  void play(String path, Surface surface);
}
