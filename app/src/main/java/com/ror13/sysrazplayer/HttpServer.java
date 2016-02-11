package com.ror13.sysrazplayer;

/**
 * Created by ror13 on 2/10/16.
 */
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.util.Enumeration;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import org.apache.http.HttpEntity;
import org.apache.http.HttpException;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.entity.ContentProducer;
import org.apache.http.entity.EntityTemplate;
import org.apache.http.impl.DefaultConnectionReuseStrategy;
import org.apache.http.impl.DefaultHttpResponseFactory;
import org.apache.http.impl.DefaultHttpServerConnection;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.protocol.BasicHttpContext;
import org.apache.http.protocol.BasicHttpProcessor;
import org.apache.http.protocol.HttpContext;
import org.apache.http.protocol.HttpRequestHandler;
import org.apache.http.protocol.HttpRequestHandlerRegistry;
import org.apache.http.protocol.HttpService;
import org.apache.http.protocol.ResponseConnControl;
import org.apache.http.protocol.ResponseContent;
import org.apache.http.protocol.ResponseDate;
import org.apache.http.protocol.ResponseServer;

public class HttpServer  extends Thread {

    ServerSocket serverSocket;
    Socket socket;
    HttpService httpService;
    BasicHttpContext basicHttpContext;
    static final int HttpServerPORT = 8080;
    boolean RUNNING = false;


    HttpServer() {
        RUNNING = true;
        startHttpService();

    }

    @Override
    public void run() {

        try {
            serverSocket = new ServerSocket(HttpServerPORT);
            serverSocket.setReuseAddress(true);

            while (RUNNING) {
                socket = serverSocket.accept();
                DefaultHttpServerConnection httpServerConnection = new DefaultHttpServerConnection();
                httpServerConnection.bind(socket, new BasicHttpParams());
                httpService.handleRequest(httpServerConnection,
                        basicHttpContext);
                httpServerConnection.shutdown();
            }
            serverSocket.close();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (HttpException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    private synchronized void startHttpService() {
        BasicHttpProcessor basicHttpProcessor = new BasicHttpProcessor();
        basicHttpContext = new BasicHttpContext();

        basicHttpProcessor.addInterceptor(new ResponseDate());
        basicHttpProcessor.addInterceptor(new ResponseServer());
        basicHttpProcessor.addInterceptor(new ResponseContent());
        basicHttpProcessor.addInterceptor(new ResponseConnControl());

        httpService = new HttpService(basicHttpProcessor,
                new DefaultConnectionReuseStrategy(),
                new DefaultHttpResponseFactory());

        HttpRequestHandlerRegistry registry = new HttpRequestHandlerRegistry();
        registry.register("/", new HomeCommandHandler());
        httpService.setHandlerResolver(registry);
    }

    public synchronized void stopServer() {
        RUNNING = false;
        if (serverSocket != null) {
            try {
                serverSocket.close();
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }

    class HomeCommandHandler implements HttpRequestHandler {
        @Override
        public void handle(HttpRequest request, HttpResponse response,
                           HttpContext httpContext) throws HttpException, IOException {



            HttpEntity httpEntity = new EntityTemplate(
                    new ContentProducer() {

                        public void writeTo(final OutputStream outstream)
                                throws IOException {
                            Logcat logCat = new Logcat();
                            OutputStreamWriter outputStreamWriter = new OutputStreamWriter(
                                    outstream, "UTF-8");
                            String response = "<html><head></head><body>" +
                                    "<script type=\"text/javascript\">\n" +
                                    "window.setTimeout(function(){ document.location.reload(true); }, 1000);\n" +
                                    "</script>" +
                                    logCat.getSystemLog() +
                                    "</body></html>";

                            outputStreamWriter.write(response);
                            outputStreamWriter.flush();
                        }
                    });
            response.setHeader("Content-Type", "text/html");
            response.setEntity(httpEntity);
        }

    }

}

class Logcat{


    public String getSystemLog(){
        StringBuilder logCat = new StringBuilder();
        logCat.append("=======update======<br>");
        try{
            Process process = Runtime.getRuntime().exec("logcat -t 10");
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

