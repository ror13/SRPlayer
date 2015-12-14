package com.ror13.sysrazplayer;

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
        System.loadLibrary("CFfmpeg");
    }
    public void run() {
        play(path, surface);
    }
    public native String tst();
    public native  void play(String path, Surface surface);
}
