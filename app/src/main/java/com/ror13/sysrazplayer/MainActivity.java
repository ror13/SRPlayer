package com.ror13.sysrazplayer;

import android.app.Activity;
import android.content.Intent;
import android.content.res.Resources;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.ArrayAdapter;
import android.widget.Toast;

import org.w3c.dom.Text;

public class MainActivity extends Activity  {


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);


        Config config = Config.getInstance();
        //Resources res = getResources();

        setContentView(R.layout.activity_main);
        //setContentView(mSurfaceView);




        Button btnPlay = (Button) findViewById(R.id.buttonPlay);
        btnPlay.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, CPlayer.class);
                startActivity(intent);
            }
        });

        Button btnUri = (Button) findViewById(R.id.buttonUri);
        ((TextView)findViewById(R.id.textViewUri)).setText(config.mUri);
        btnUri.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                OnEndConfig onEnd = new OnEndConfig() {
                    @Override
                    public void onEndConfig() {
                        Config config = Config.getInstance();
                        config.mUri = MenuFileDialog.getInstance(null, null).getUri();
                        TextView tUri = (TextView) findViewById(R.id.textViewUri);
                        tUri.setText(config.mUri);
                    }
                };

                MenuFileDialog menu = MenuFileDialog.getInstance(MainActivity.this, onEnd);
                menu.show();


            }
        });



        Spinner spinRtspProtocolType = (Spinner) findViewById(R.id.spinner_rtsp_protocol_type);
        int pos = ((ArrayAdapter)spinRtspProtocolType.getAdapter()).getPosition(config.mRtspProtocol);
        ((Spinner)findViewById(R.id.spinner_rtsp_protocol_type)).setSelection(pos);
        spinRtspProtocolType.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view,
                                       int position, long id) {
                parent.getItemAtPosition(position);

                Object obj = parent.getItemAtPosition(position);
                if (obj != null) {
                    Config config = Config.getInstance();
                    config.mRtspProtocol = obj.toString();
                }

            }

            @Override
            public void onNothingSelected(AdapterView<?> arg0) {
            }
        });

        Spinner spinSkipPacket = (Spinner) findViewById(R.id.spinner_packet_buffer_size);
        pos = ((ArrayAdapter)spinSkipPacket.getAdapter()).getPosition(Integer.toString(config.mPacketBufferSize));
        ((Spinner)findViewById(R.id.spinner_packet_buffer_size)).setSelection(pos);
        //spinSkipPacket.setSelection(1,false);
        spinSkipPacket.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view,
                                       int position, long id) {
                parent.getItemAtPosition(position);

                Object obj = parent.getItemAtPosition(position);
                if (obj != null) {
                    Config config = Config.getInstance();
                    config.mPacketBufferSize = Integer.parseInt(obj.toString());
                }

            }

            @Override
            public void onNothingSelected(AdapterView<?> arg0) {
            }
        });

        CheckBox cbSkipPacket = (CheckBox) findViewById(R.id.checkBoxSkipPacket);
        cbSkipPacket.setChecked(config.mIsSkipPacket);
        cbSkipPacket.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                Config config = Config.getInstance();
                config.mIsSkipPacket = b;
            }
        });

        CheckBox cbMaxFps = (CheckBox) findViewById(R.id.checkBoxMaxFps);
        cbMaxFps.setChecked(config.mIsMaxFps);
        cbMaxFps.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                Config config = Config.getInstance();
                config.mIsMaxFps = b;
            }
        });

        CheckBox cbFlushStream = (CheckBox) findViewById(R.id.checkBoxFlushStream);
        cbFlushStream.setChecked(config.mIsFlush);
        cbFlushStream.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                Config config = Config.getInstance();
                config.mIsFlush = b;
            }
        });






    }



    @Override
    public void onBackPressed() {
        // do nothing because you don't want them to leave when it's pressed
    }

    @Override
    protected void onDestroy() {
        // Unregister since the activity is about to be closed.
        super.onDestroy();
    }


}
