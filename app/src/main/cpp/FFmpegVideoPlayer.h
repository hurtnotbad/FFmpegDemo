//
// Created by Lammy on 2018/9/2.
//

#ifndef FFMPEGDEMO_FFMPEGVIDEOPLAYER_H
#define FFMPEGDEMO_FFMPEGVIDEOPLAYER_H

#include "String"
#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window.h>

class FFmpegVideoPlayer {

private:
    char * videoPath;
    void setVideoPath(char *videoPath);


public:
    FFmpegVideoPlayer();
    ~FFmpegVideoPlayer();


  void pause();
  void stop();
  void play( const char *path , ANativeWindow* aNativeWindow);


};


#endif //FFMPEGDEMO_FFMPEGVIDEOPLAYER_H
