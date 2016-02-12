package com.ror13.sysrazplayer;

import android.app.Application;

/**
 * Created by ror131 on 2/12/16.
 */
public class SysRazApp extends Application {
    private static SysRazApp instance;

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
    }

    public static SysRazApp getInstance() {
        return instance;
    }
}
