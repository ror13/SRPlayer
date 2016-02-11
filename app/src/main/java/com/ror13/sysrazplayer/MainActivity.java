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
import android.widget.RadioButton;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.ArrayAdapter;
import android.widget.Toast;
import org.w3c.dom.Text;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;

public class MainActivity extends Activity  {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);

        Config config = Config.getInstance();
        //Resources res = getResources();

        setContentView(R.layout.activity_main);
        //setContentView(mSurfaceView);


        TextView tvIp = (TextView) findViewById(R.id.textViewIp);
        tvIp.setText(getIpAddress());

        Button btnSysConfig = (Button) findViewById(R.id.buttonSysConfig);
        btnSysConfig.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivityForResult(new Intent(android.provider.Settings.ACTION_SETTINGS), 0);

            }
        });

        Button btnPlay = (Button) findViewById(R.id.buttonPlay);
        btnPlay.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, CPlayer.class);
                startActivity(intent);
            }
        });

        Button btnUri = (Button) findViewById(R.id.buttonUri);
        ((TextView)findViewById(R.id.textViewUri)).setText(config.get(Config.OPT_URI).toString());
        btnUri.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                OnEndConfig onEnd = new OnEndConfig() {
                    @Override
                    public void onEndConfig() {
                        Config config = Config.getInstance();
                        String uri = MenuFileDialog.getInstance(null, null).getUri();
                        TextView tUri = (TextView) findViewById(R.id.textViewUri);
                        tUri.setText(uri);
                        config.put(Config.OPT_URI, uri);
                    }
                };

                MenuFileDialog menu = MenuFileDialog.getInstance(MainActivity.this, onEnd);
                menu.show();


            }
        });



        Spinner spinRtspProtocolType = (Spinner) findViewById(R.id.spinner_rtsp_protocol_type);
        int pos = ((ArrayAdapter)spinRtspProtocolType.getAdapter()).getPosition(config.get(Config.OPT_RTSP_PROTOCOL).toString());
        ((Spinner)findViewById(R.id.spinner_rtsp_protocol_type)).setSelection(pos);
        spinRtspProtocolType.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view,
                                       int position, long id) {
                parent.getItemAtPosition(position);

                Object obj = parent.getItemAtPosition(position);
                if (obj != null) {
                    Config config = Config.getInstance();
                    config.put(Config.OPT_RTSP_PROTOCOL, obj.toString());
                }

            }

            @Override
            public void onNothingSelected(AdapterView<?> arg0) {
            }
        });

        Spinner spinSkipPacket = (Spinner) findViewById(R.id.spinner_packet_buffer_size);
        pos = ((ArrayAdapter)spinSkipPacket.getAdapter()).getPosition(Integer.toString(config.getValInt(Config.OPT_PACKET_BUFFER_SIZE)));
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
                    config.put(Config.OPT_PACKET_BUFFER_SIZE, Integer.parseInt(obj.toString()));
                }

            }

            @Override
            public void onNothingSelected(AdapterView<?> arg0) {
            }
        });

        CheckBox cbSkipPacket = (CheckBox) findViewById(R.id.checkBoxSkipPacket);
        cbSkipPacket.setChecked(config.getValBool(Config.OPT_IS_SKIP_PACKET));
        cbSkipPacket.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                Config config = Config.getInstance();
                config.put(Config.OPT_IS_SKIP_PACKET, b);
            }
        });

        CheckBox cbMaxFps = (CheckBox) findViewById(R.id.checkBoxMaxFps);
        cbMaxFps.setChecked(config.getValBool(Config.OPT_IS_MAX_FPS));
        cbMaxFps.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                Config config = Config.getInstance();
                config.put(Config.OPT_IS_MAX_FPS ,b);
            }
        });

        CheckBox cbFlushStream = (CheckBox) findViewById(R.id.checkBoxFlushStream);
        cbFlushStream.setChecked(config.getValBool(Config.OPT_IS_FLUSH));
        cbFlushStream.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                Config config = Config.getInstance();
                config.put(Config.OPT_IS_FLUSH, b);
            }
        });

        CheckBox cbLoopPlaying = (CheckBox) findViewById(R.id.checkBoxLoopPlaying);
        cbLoopPlaying.setChecked(config.getValBool(Config.OPT_IS_LOOP_PLAYING));
        cbLoopPlaying.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                Config config = Config.getInstance();
                config.put(Config.OPT_IS_LOOP_PLAYING, b);
            }
        });


        RadioButton rbGlesRender = (RadioButton) findViewById(R.id.radioButtonGlesRender);
        rbGlesRender.setChecked(config.getValBool(Config.OPT_IS_WINDOW_GLES));
        rbGlesRender.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                Config config = Config.getInstance();
                config.put(Config.OPT_IS_WINDOW_GLES, b);
            }
        });

        RadioButton rbNativeRender = (RadioButton) findViewById(R.id.radioButtonNativeRender);
        rbNativeRender.setChecked(config.getValBool(Config.OPT_IS_WINDOW_NATIVE));
        rbNativeRender.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                Config config = Config.getInstance();
                config.put(Config.OPT_IS_WINDOW_NATIVE, b);
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

    private String getIpAddress() {
        String ip = "Ip address : ";
        try {
            Enumeration<NetworkInterface> enumNetworkInterfaces = NetworkInterface
                    .getNetworkInterfaces();
            while (enumNetworkInterfaces.hasMoreElements()) {
                NetworkInterface networkInterface = enumNetworkInterfaces
                        .nextElement();
                Enumeration<InetAddress> enumInetAddress = networkInterface
                        .getInetAddresses();
                while (enumInetAddress.hasMoreElements()) {
                    InetAddress inetAddress = enumInetAddress.nextElement();

                    if (inetAddress.isSiteLocalAddress()) {
                        ip += "" + inetAddress.getHostAddress();
                    }

                }

            }

        } catch (SocketException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            ip += "Something Wrong! " + e.toString() + "\n";
        }

        return ip;
    }

}
