package com.ror13.sysrazplayer;

import android.media.MediaCodec;
import android.media.MediaCodec.BufferInfo;
import android.media.MediaExtractor;
import android.view.Surface;
import android.media.MediaFormat;
import android.util.*;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.lang.Thread;






/**
 * Created by ror131 on 12/8/15.
 */
public class SysRazPlayer {
    private MediaExtractor extractor;
    private MediaCodec decoder;
    private Surface surface;
    public void run(String path) {
        extractor = new MediaExtractor();
        try {
            extractor.setDataSource(path);
        } catch (IOException e) {
            Log.e("DEBUG", "IOException ");
            e.printStackTrace();
        }

        for (int i = 0; i < extractor.getTrackCount(); i++) {
            MediaFormat format = extractor.getTrackFormat(i);
            String mime = format.getString(MediaFormat.KEY_MIME);
            if (mime.startsWith("video/")) {
                extractor.selectTrack(i);
                try {
                    decoder = MediaCodec.createDecoderByType(mime);
                } catch (IOException e) {
                    e.printStackTrace();
                }
                decoder.configure(format, surface, null, 0);
                break;
            }
        }

        if (decoder == null) {
            Log.e("DecodeActivity", "Can't find video info!");
            return;
        }

        decoder.start();

        ByteBuffer[] inputBuffers = decoder.getInputBuffers();
        ByteBuffer[] outputBuffers = decoder.getOutputBuffers();
        BufferInfo info = new BufferInfo();
        boolean isEOS = false;
        long startMs = System.currentTimeMillis();

        while (!Thread.interrupted()) {
            if (!isEOS) {
                int inIndex = decoder.dequeueInputBuffer(10000);
                if (inIndex >= 0) {
                    ByteBuffer buffer = inputBuffers[inIndex];
                    int sampleSize = extractor.readSampleData(buffer, 0);
                    if (sampleSize < 0) {
                        Log.d("DecodeActivity", "InputBuffer BUFFER_FLAG_END_OF_STREAM");
                        decoder.queueInputBuffer(inIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                        isEOS = true;
                    } else {
                        decoder.queueInputBuffer(inIndex, 0, sampleSize, extractor.getSampleTime(), 0);
                        extractor.advance();
                    }
                }
            }

            int outIndex = decoder.dequeueOutputBuffer(info, 10000);
            switch (outIndex) {
                case MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED:
                    Log.d("DecodeActivity", "INFO_OUTPUT_BUFFERS_CHANGED");
                    outputBuffers = decoder.getOutputBuffers();
                    break;
                case MediaCodec.INFO_OUTPUT_FORMAT_CHANGED:
                    Log.d("DecodeActivity", "New format " + decoder.getOutputFormat());
                    break;
                case MediaCodec.INFO_TRY_AGAIN_LATER:
                    Log.d("DecodeActivity", "dequeueOutputBuffer timed out!");
                    break;
                default:
                    ByteBuffer buffer = outputBuffers[outIndex];
                    Log.v("DecodeActivity", "We can't use this buffer but render it due to the API limit, " + buffer);

                    // We use a very simple clock to keep the video FPS, or the video
                    // playback will be too fast
                    while (info.presentationTimeUs / 1000 > System.currentTimeMillis() - startMs) {
                        try {
                            Thread.sleep(10);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                            break;
                        }
                    }
                    decoder.releaseOutputBuffer(outIndex, true);
                    break;
            }

            if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                Log.d("DecodeActivity", "OutputBuffer BUFFER_FLAG_END_OF_STREAM");
                break;
            }
        }

        decoder.stop();
        decoder.release();
        extractor.release();
    }
}
