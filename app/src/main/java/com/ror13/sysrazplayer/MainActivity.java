package com.ror13.sysrazplayer;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Button;

public class MainActivity extends Activity  {

    public static final String CUSTOM_EVENT= "main-action-event";
    SurfaceView mSurfaceView;
    Menu menu;
    CPlayer player;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);

        player = new CPlayer();
        mSurfaceView = new SurfaceView(this);

        setContentView(R.layout.activity_main);
        setContentView(mSurfaceView);

        menu =  Menu.getInstance(this, new OnEndConfig() {
            @Override
            public void onEndConfig() {
                /*SysRazPlayer player = new SysRazPlayer();
                player.setPath();
                player.setSurface(MainActivity.this.mSurfaceView.getHolder().getSurface());
                player.start();
                MainActivity.this.player.open(Menu.getInstance(null, null).getUri(),
                        MainActivity.this.mSurfaceView.getHolder().getSurface(),
                        true);
                MainActivity.this.player.start();
                */
                playVideo();
            }
        });

/*
        Button btnPlay = (Button) findViewById(R.id.buttonPlay);
        btnPlay.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                playVideo();
            }
        });
*/



        menu.show();
    }

    public void playVideo(){
        //MainActivity.this.player.open(MainActivity.this.menu.getUri(),
        Log.d("JAVA", "0");
        setContentView(NULL);
        setContentView(mSurfaceView);

        Log.d("JAVA", "1");
        player.open("/storage/sdcard0/Download/startrekintodarkness-usajj60sneak_h1080p.mov",
                mSurfaceView.getHolder().getSurface(),
                true);
        Log.d("JAVA", "2");
        player.start();
        Log.d("JAVA", "3");

        Log.d("JAVA", "4");
    }

    @Override
    protected void onDestroy() {
        // Unregister since the activity is about to be closed.
        super.onDestroy();
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
                        MainActivity.this.setContentView(R.layout.activity_main);
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
        builder.setMessage("Stop play video?").setPositiveButton("Yes", dialogClickListener)
                .setNegativeButton("No", dialogClickListener).show();

        //menu.show();
        //SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surface);

        //player.run("/storage/sdcard0/Download/big_buck_bunny_720p_50mb.mp4", mSurfaceView.getHolder().getSurface());
        return true;
    }


}
