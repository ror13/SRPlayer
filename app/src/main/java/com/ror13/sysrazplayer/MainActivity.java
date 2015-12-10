package com.ror13.sysrazplayer;

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

import java.nio.ByteBuffer;

public class MainActivity extends Activity  implements SurfaceHolder.Callback{
    static final int OUTPUT_WIDTH = 640;
    static final int OUTPUT_HEIGHT = 480;


    SurfaceView mSurfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mSurfaceView = (SurfaceView) findViewById(R.id.surface);
        mSurfaceView.getHolder().addCallback(this);

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

        SysRazPlayer player = new SysRazPlayer();
      //  player.run("/storage/sdcard0/Download/big_buck_bunny_240p_30mb.mp4",mSurfaceView.getHolder().getSurface());
        player.run("/storage/sdcard0/Download/big_buck_bunny_720p_50mb.mp4",mSurfaceView.getHolder().getSurface());


        return true;
    }
}
