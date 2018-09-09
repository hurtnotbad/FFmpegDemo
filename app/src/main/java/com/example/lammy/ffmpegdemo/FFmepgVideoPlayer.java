package com.example.lammy.ffmpegdemo;

import android.graphics.PixelFormat;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.VideoView;

/**
 * Created by Lammy on 2018/9/2.
 */

public class FFmepgVideoPlayer {

    private SurfaceView surfaceView;
    public FFmepgVideoPlayer(SurfaceView surfaceView){
        this.surfaceView = surfaceView;
        IFFmpEgVideoPlayer = getPlayer();
    }

    public   void pause(){
        pause(IFFmpEgVideoPlayer);
    }
    public  void stop(){
        stop( IFFmpEgVideoPlayer);
    }
    public  void play(String path){

        SurfaceHolder surfaceHolder = surfaceView.getHolder();
        surfaceHolder.setFormat(PixelFormat.RGBA_8888);

        Surface surface = surfaceHolder.getSurface();
        if(surface == null){
            Log.e("lammy", "surface is null");
        }

        play(IFFmpEgVideoPlayer , path ,  surfaceHolder.getSurface());
    }


    private long IFFmpEgVideoPlayer;
    private native long getPlayer();
    private native void pause(long IFFmepgVideoPlayer);
    private native void stop(long IFFmepgVideoPlayer);
    private native void play(long IFFmepgVideoPlayer , String path , Surface surface);




    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("yuv");
//        System.loadLibrary("yuv_utils");
        System.loadLibrary("avcodec-57");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("avformat-57");
        System.loadLibrary("avutil-55");
        System.loadLibrary("swresample-2");
        System.loadLibrary("swscale-4");
    }

}
