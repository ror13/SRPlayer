package com.ror13.sysrazplayer;

import android.app.Activity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.MotionEvent;
import android.view.Window;

public class MainActivity extends Activity  implements SurfaceHolder.Callback{

    public static final String CUSTOM_EVENT= "main-action-event";
    SurfaceView mSurfaceView;
    Menu menu;
    CPlayer player;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.activity_main);



        mSurfaceView = (SurfaceView) findViewById(R.id.surface);
        mSurfaceView.getHolder().addCallback(this);
        player = new CPlayer();
        menu =  Menu.getInstance(this, new OnEndConfig() {
            @Override
            public void onEndConfig() {
                /*SysRazPlayer player = new SysRazPlayer();
                player.setPath();
                player.setSurface(MainActivity.this.mSurfaceView.getHolder().getSurface());
                player.start();                */
                MainActivity.this.player.open(Menu.getInstance(null, null).getUri(),
                        MainActivity.this.mSurfaceView.getHolder().getSurface(),
                        true);
                MainActivity.this.player.start();
            }
        });
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
        super.onTouchEvent(event);

        menu.show();
        //SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surface);

        //player.run("/storage/sdcard0/Download/big_buck_bunny_720p_50mb.mp4", mSurfaceView.getHolder().getSurface());
        return true;
    }


}
