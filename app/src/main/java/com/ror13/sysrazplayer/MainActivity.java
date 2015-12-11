package com.ror13.sysrazplayer;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.MediaPlayer;
import android.media.MediaRecorder;
import android.app.Activity;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
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

        LocalBroadcastManager.getInstance(this).registerReceiver(mMessageReceiver,
                new IntentFilter(CUSTOM_EVENT));
    }
    private BroadcastReceiver mMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            // Get extra data included in the Intent
            String message = intent.getStringExtra(CUSTOM_EVENT);
            //Log.d("receiver", "Got message: " + message);
            SysRazPlayer player = new SysRazPlayer();
            //player.run(message, mSurfaceView.getHolder().getSurface());
            //player.run("http://mirror.cessen.com/blender.org/peach/trailer/trailer_1080p.mov", mSurfaceView.getHolder().getSurface());
            MediaPlayer mp = new MediaPlayer();
            try {
                mp.setDataSource("rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov");
                mp.setSurface(mSurfaceView.getHolder().getSurface());
                mp.prepare();
                mp.start();
            } catch (IllegalArgumentException e) {
                e.printStackTrace();
            } catch (IllegalStateException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }
            //player.run("rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov", mSurfaceView.getHolder().getSurface());
        }
    };
    @Override
    protected void onDestroy() {
        // Unregister since the activity is about to be closed.
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mMessageReceiver);
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

        SysRazPlayer player = new SysRazPlayer();
      //  player.run("/storage/sdcard0/Download/big_buck_bunny_240p_30mb.mp4",mSurfaceView.getHolder().getSurface());
      //  player.run("/storage/sdcard0/Download/big_buck_bunny_720p_50mb.mp4",mSurfaceView.getHolder().getSurface());

        Menu menu =  Menu.getInstance(this);
        menu.show();

        //player.run("/storage/sdcard0/Download/big_buck_bunny_720p_50mb.mp4", mSurfaceView.getHolder().getSurface());
        return true;
    }
}
