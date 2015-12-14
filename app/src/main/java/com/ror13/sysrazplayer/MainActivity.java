package com.ror13.sysrazplayer;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.MediaPlayer;
import android.media.MediaRecorder;
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.media.MediaCodec;
import android.os.Bundle;
import android.text.TextPaint;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.MotionEvent;

import java.io.IOException;
import java.nio.ByteBuffer;

public class MainActivity extends Activity  implements SurfaceHolder.Callback{

    public static final String CUSTOM_EVENT= "main-action-event";
    SurfaceView mSurfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mSurfaceView = (SurfaceView) findViewById(R.id.surface);
        mSurfaceView.getHolder().addCallback(this);
        Menu menu =  Menu.getInstance(this);
        menu.show();
    }

    @Override
    protected void onDestroy() {
        // Unregister since the activity is about to be closed.
        super.onDestroy();
    }
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        // surface is fully initialized on the activity
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        Log.e("tst debug:::", "1");
        super.onTouchEvent(event);
        Log.e("tst debug:::", "2");


        //SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surface);
        CFfmpeg hj = new CFfmpeg();
        Log.e("fdf  1", hj.tst());
        hj.setPath("/storage/sdcard0/Download/big_buck_bunny_720p_50mb.mp4");
        hj.setSurface(mSurfaceView.getHolder().getSurface());
        hj.play("/storage/sdcard0/Download/big_buck_bunny_720p_50mb.mp4", mSurfaceView.getHolder().getSurface());

        //hj.start();
        Log.e("fdf  2", hj.tst());
      //  player.run("/storage/sdcard0/Download/big_buck_bunny_240p_30mb.mp4",mSurfaceView.getHolder().getSurface());
      //  player.run("/storage/sdcard0/Download/big_buck_bunny_720p_50mb.mp4",mSurfaceView.getHolder().getSurface());

        Menu menu =  Menu.getInstance(this);
        menu.show();

        //player.run("/storage/sdcard0/Download/big_buck_bunny_720p_50mb.mp4", mSurfaceView.getHolder().getSurface());
        return true;
    }
}
