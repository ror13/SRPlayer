package com.ror13.sysrazplayer;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.View;
import android.widget.EditText;

/**
 * Created by ror13 on 13.12.15.
 */


public class CPlayer extends Activity{
    SurfaceView mSurfaceView;
    public CPlayer(){

    }
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mSurfaceView = new SurfaceView(getBaseContext());
        setContentView(mSurfaceView);

    }

    @Override
    protected void onStart() {
        super.onStart();
        new Thread(new Runnable() {
            public void run(){
                SystemClock.sleep(1000);
                Config config= Config.getInstance();
                CPlayer.this.open(config, mSurfaceView.getHolder().getSurface());
                CPlayer.this.start();
            }
        }).start();
    }

    @Override
    protected void onStop() {
        super.onStop();
        close();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        super.onTouchEvent(event);
        if(event.getAction() != MotionEvent.ACTION_UP){
            return true;
        }

        DialogInterface.OnClickListener dialogClickListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                switch (which){
                    case DialogInterface.BUTTON_POSITIVE:
                        Intent intent = new Intent(CPlayer.this, MainActivity.class);
                        startActivity(intent);
                        //Yes button clicked
                        break;

                    case DialogInterface.BUTTON_NEGATIVE:
                        //No button clicked
                        dialog.cancel();
                        break;
                }
            }
        };

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setMessage("Stop video?").setPositiveButton("Yes", dialogClickListener)
                .setNegativeButton("No", dialogClickListener).show();

        return true;
    }


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
    public native  void open(Object config, Surface surface);
    public native  void close();
    public native  void start();
    public native  void stop();
}
