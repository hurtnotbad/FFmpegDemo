package com.example.lammy.ffmpegdemo;

/**
 * Created by Lammy on 2018/9/1.
 */

public class FFmpegUtil {
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("avcodec-57");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("avformat-57");
        System.loadLibrary("avutil-55");
        System.loadLibrary("swresample-2");
        System.loadLibrary("swscale-4");
    }

    public native static void decode(String input,String output);




}
