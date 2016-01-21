package com.ror13.sysrazplayer;

import android.os.Build;
import android.util.Log;
import android.view.Surface;

/**
 * Created by ror13 on 13.12.15.
 */


public class CPlayer{

    long pointerToPlayer; //using from C++ code

    static {
        int currentapiVersion = android.os.Build.VERSION.SDK_INT;
        if (currentapiVersion > Build.VERSION_CODES.KITKAT){
            System.loadLibrary("CPlayer_v5");
        } else{
            System.loadLibrary("CPlayer_v4");
        }

    }

    public native String tst();
    public native  void open(String path, Surface surface, boolean isStream);
    public native  void close();
    public native  void start();
    public native  void stop();
}
