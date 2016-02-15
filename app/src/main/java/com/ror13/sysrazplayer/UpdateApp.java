package com.ror13.sysrazplayer;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Environment;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import android.content.pm.PackageManager;

/**
 * Created by ror131 on 2/15/16.
 */
public class UpdateApp {

    public static void update(String path) {
        try {
            //Log.e("UpdateAPP", "======================= 0" );
            String outFilePath = Environment.getExternalStorageDirectory() + "/update.apk";
            File tmpFile = new File( path);
            FileInputStream fis =  new FileInputStream(tmpFile);
            FileOutputStream fos =  new FileOutputStream(outFilePath);

            byte buff[] = new byte[1024];
            while(fis.available() >= 0){
                int count = fis.read(buff);
                if(count <= 0){
                    break;
                }
                fos.write(buff,0,count);

            }
            /*
            Log.e("UpdateAPP", "======================= 1" );

            Process process = Runtime.getRuntime().exec("pm install -r " + outFilePath);
            BufferedReader bufferedReader = new BufferedReader(
                    new InputStreamReader(process.getErrorStream()));
            String line ;
            while ((line = bufferedReader.readLine()) != null) {
                Log.e("pm install -r:", line);
            }
            Log.e("UpdateAPP", "======================= 2" );
            */
            //PackageManager pm = SysRazApp.getInstance().getBaseContext().getPackageManager();
            //PackageInstallObserver obs = new PackageInstallObserver();
            //method = pm.getClass().getMethod("installPackage", types);



            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setDataAndType(Uri.fromFile(new File(outFilePath)), "application/vnd.android.package-archive");
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK); // without this flag android returned a intent error!
            Context context = SysRazApp.getInstance().getBaseContext();
            context.startActivity(intent);


        } catch (Exception e) {
            Log.e("UpdateAPP", "Update error! " + e.getMessage());
        }

    }
}

