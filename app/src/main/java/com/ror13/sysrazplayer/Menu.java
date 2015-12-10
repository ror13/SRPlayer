package com.ror13.sysrazplayer;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;

import java.net.ContentHandler;

/**
 * Created by ror131 on 12/10/15.
 */
public class Menu extends Dialog implements View.OnClickListener{
    public Menu (Context context){
        super(context);
        setContentView(R.layout.menu);
        findViewById(R.id.play).setOnClickListener(this);
        findViewById(R.id.open).setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        switch(v.getId()){
            case R.id.play : dismiss(); break;
            case R.id.open : selectFile(); break;
        }
    }

    private void selectFile(){

    }
}
