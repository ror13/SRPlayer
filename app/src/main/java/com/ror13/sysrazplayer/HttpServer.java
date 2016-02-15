package com.ror13.sysrazplayer;

import android.content.Context;
import android.content.res.AssetManager;
import android.net.Uri;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URL;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.logging.Level;

import fi.iki.elonen.NanoHTTPD;

/**
 * Created by ror13 on 2/10/16.
 */

public class HttpServer extends NanoHTTPD {
    private AssetManager assetManager;
    private List<String> assetFileList;
    public HttpServer() throws IOException {
        super(8080);
        Context appContext = SysRazApp.getInstance().getBaseContext();
        assetManager = appContext.getAssets();
        assetFileList = Arrays.asList(assetManager.list(""));
        loadMimeTypes();
    }

    private  void loadMimeTypes() {
        if (MIME_TYPES == null) {
            MIME_TYPES = new HashMap<String, String>();
        }
        try {
            Properties properties = new Properties();
            InputStream stream = assetManager.open("mimetypes.properties");
            properties.load(stream);
            stream.close();
            MIME_TYPES.putAll((Map)properties);

        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    @Override
    public Response serve(IHTTPSession session) {
        try {

            InputStream msg = null;
            String uri = session.getUri().substring(1);
            if(uri.equals("")){
                uri = "index.html";
            }

            if(!assetFileList.contains(uri)){
                throw new IOException("NOT FOUND URI :" + uri);
            }

            if (session.getMethod().equals(Method.GET)) {

                msg = assetManager.open(uri);

            }

            if (session.getMethod().equals(Method.POST)) {
                HashMap<String, String> parms= new HashMap<String, String>();
                session.parseBody(parms);
                //System.out.println(session.getMethod() + " " + session.getParms());
                if(parms.keySet().contains("fileToUpload")){
                    String loaction = parms.get("fileToUpload");
                    File tmpFile = new File( loaction);
                    FileInputStream is =  new FileInputStream(tmpFile);
                    byte buff[] = new byte[10];
                    is.read(buff);
                    System.out.println("1  " + new String(buff));
                    is.read(buff);
                    System.out.println("2  "  + new String(buff));
                }

                Log.d("values", session.getParms().values().toString());
                Log.d("keys", session.getParms().keySet().toString());

                String log = null;// = new Logcat().getSystemLog();
                try {
                    JSONObject jsonObj = new JSONObject();
                    jsonObj.put("log", log);
                    msg = new ByteArrayInputStream(jsonObj.toString().getBytes());
                } catch (JSONException ex) {
                    ex.printStackTrace();
                }

            }

            return newFixedLengthResponse(Response.Status.OK, getMimeTypeForFile(uri), msg, msg.available());
        } catch (Exception e) {
            e.printStackTrace();
        }
        return newFixedLengthResponse(Response.Status.NO_CONTENT, null, null, 0L );
    }
}


class Logcat{


    public String getSystemLog(){
        StringBuilder logCat = new StringBuilder();
        try{
            Process process = Runtime.getRuntime().exec("logcat -d");
            BufferedReader bufferedReader = new BufferedReader(
                    new InputStreamReader(process.getInputStream()));
            String line ;
            while ((line = bufferedReader.readLine()) != null) {
                logCat.append(line +"<br>");
            }
        } catch (IOException e) {
        }

        return logCat.toString();
    }



}

