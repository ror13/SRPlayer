package com.ror13.sysrazplayer;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import java.io.*;
import android.os.*;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        Log.e("tst debug:::", "1");
        super.onTouchEvent(event);
        Log.e("tst debug:::", "2");

        FileDialog fileDialog = new FileDialog( );
        Log.e("tst debug:::", fileDialog.toString());
        return true;
    }
}
