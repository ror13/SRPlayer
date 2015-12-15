package com.ror13.sysrazplayer;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.EditText;

import java.net.ContentHandler;

/**
 * Created by ror131 on 12/10/15.
 */
interface OnEndConfig {
    void onEndConfig();
}

public class Menu extends Dialog implements View.OnClickListener{
    private static Menu singleton;
    private OnEndConfig onEndConfig;
    private static Context mContext;
    String mUri;
    public Menu(){
        super(mContext);
        setContentView(R.layout.menu);
        findViewById(R.id.play).setOnClickListener(this);
        findViewById(R.id.open).setOnClickListener(this);
        EditText et = (EditText) findViewById(R.id.editText);


        et.setText("rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov");
    }

    public static Menu getInstance(Context context, OnEndConfig onEndConfig){
        if (singleton == null){
            mContext = context;
            singleton = new Menu();
            singleton.onEndConfig = onEndConfig;
        }
        return singleton;
    }

    @Override
    public void onClick(View v) {
        switch(v.getId()){
            case R.id.play : dismiss();
                EditText et = (EditText) findViewById(R.id.editText);
                mUri = et.getText().toString();
                if (onEndConfig != null)
                    onEndConfig.onEndConfig();
                break;
            case R.id.open : selectFile(); break;
        }
    }

    private void selectFile(){

    }

    public String getUri(){
        return mUri;
    }
}
