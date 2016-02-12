package com.ror13.sysrazplayer;

import android.app.Application;

import java.io.IOException;

/**
 * Created by ror131 on 2/12/16.
 */
public class SysRazApp extends Application {
    private static SysRazApp instance;
    private static HttpServer httpServer;

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
        try {
            httpServer = new HttpServer();
            httpServer.start();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static SysRazApp getInstance() {
        return instance;
    }
}
