package com.ror13.sysrazplayer;

import android.content.Context;
import android.content.res.AssetManager;
import android.net.Uri;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Map;

import fi.iki.elonen.NanoHTTPD;

/**
 * Created by ror13 on 2/10/16.
 */

public class HttpServer extends NanoHTTPD {

    public HttpServer() {
        super(8080);
    }

    public HttpServer(String hostname, int port) {
        super(hostname, port);
    }



    @Override
    public Response serve(IHTTPSession session) {
        Method method = session.getMethod();
        //Log.d("session.getUri() ",session.getUri());
        //if (!session.getUri().equals("/")) {
        //    return newFixedLengthResponse("");
        //}
        String msg = "";

        if (session.getMethod().equals(Method.GET)) {
            Context appContext = SysRazApp.getInstance().getBaseContext();
            AssetManager assetManager = appContext.getAssets();
            try {
                InputStream input = assetManager.open("root.html");
                byte[] buffer = new byte[input.available()];
                input.read(buffer);
                input.close();
                msg = new String(buffer);
            } catch (IOException e) {
                e.printStackTrace();
            }
/*
        String msg = "<html><body><h1>Hello server</h1>\n";
        Map<String, String> parms = session.getParms();
        if (parms.get("username") == null) {
            msg += "<form action='?' method='get'>\n  <p>Your name: <input type='text' name='username'></p>\n" + "</form>\n";
        } else {
            msg += "<p>Hello, " + parms.get("username") + "!</p>";
        }
        */
        }

        if (session.getMethod().equals(Method.POST)) {
            String log = new Logcat().getSystemLog();

            try {
                JSONObject jsonObj = new JSONObject();
                jsonObj.put("log", log);
                msg = jsonObj.toString();
            } catch (JSONException ex) {
                ex.printStackTrace();
            }

        }

        return newFixedLengthResponse(msg + new Logcat().getSystemLog());
    }
}


class Logcat{


    public String getSystemLog(){
        StringBuilder logCat = new StringBuilder();
        logCat.append("=======update======<br>");
        try{
            Process process = Runtime.getRuntime().exec("logcat -d");
            BufferedReader bufferedReader = new BufferedReader(
                    new InputStreamReader(process.getInputStream()));
            String line = "";
            while ((line = bufferedReader.readLine()) != null) {
                logCat.append(line +"<br>");
            }
            logCat.toString();
        } catch (IOException e) {
        }

        return logCat.toString();
    }



}

