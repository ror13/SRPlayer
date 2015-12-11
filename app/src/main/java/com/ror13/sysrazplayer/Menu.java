package com.ror13.sysrazplayer;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
import android.view.SurfaceView;
import android.view.View;
import android.widget.EditText;

import java.net.ContentHandler;

/**
 * Created by ror131 on 12/10/15.
 */
public class Menu extends Dialog implements View.OnClickListener{
    private static Menu singleton;
    Menu(Context context){
        super(context);
        setContentView(R.layout.menu);
        findViewById(R.id.play).setOnClickListener(this);
        findViewById(R.id.open).setOnClickListener(this);
        EditText et = (EditText) findViewById(R.id.editText);
        et.setText("/system/big_buck_bunny.mp4");
    }

    public static Menu getInstance(Context context){
        if (singleton == null){
            singleton = new Menu(context);
        }
        return singleton;
    }

    @Override
    public void onClick(View v) {
        switch(v.getId()){
            case R.id.play : dismiss();
                Intent intent = new Intent(MainActivity.CUSTOM_EVENT);
                EditText et = (EditText) findViewById(R.id.editText);
                intent.putExtra(MainActivity.CUSTOM_EVENT, et.getText().toString());
                LocalBroadcastManager.getInstance(getContext()).sendBroadcast(intent);

                break;
            case R.id.open : selectFile(); break;
        }
    }

    private void selectFile(){

    }
}
