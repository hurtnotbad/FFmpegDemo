//
// Created by Lammy on 2018/9/2.
//

#include "FFmpegVideoPlayer.h"
#include <jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>



extern "C" {
JNIEXPORT jlong JNICALL
Java_com_example_lammy_ffmpegdemo_FFmepgVideoPlayer_getPlayer(JNIEnv *env, jobject instance) {
    FFmpegVideoPlayer *fFmpegVideoPlayer = new FFmpegVideoPlayer();
    return long(fFmpegVideoPlayer);
}
JNIEXPORT void JNICALL
Java_com_example_lammy_ffmpegdemo_FFmepgVideoPlayer_pause(JNIEnv *env, jobject instance,
                                                          jlong IFFmepgVideoPlayer) {
    FFmpegVideoPlayer *fFmpegVideoPlayer = (FFmpegVideoPlayer *) IFFmepgVideoPlayer;
    fFmpegVideoPlayer->pause();
}
JNIEXPORT void JNICALL
Java_com_example_lammy_ffmpegdemo_FFmepgVideoPlayer_stop(JNIEnv *env, jobject instance,
                                                         jlong IFFmepgVideoPlayer ) {
    FFmpegVideoPlayer *fFmpegVideoPlayer = (FFmpegVideoPlayer *) IFFmepgVideoPlayer;
    fFmpegVideoPlayer->stop();

}


JNIEXPORT void JNICALL
Java_com_example_lammy_ffmpegdemo_FFmepgVideoPlayer_play(JNIEnv *env,
                                                         jobject instance,
                                                         jlong IFFmepgVideoPlayer,
                                                         jstring path_,
                                                         jobject surface) {
    const char *path = env->GetStringUTFChars(path_, 0);

    // TODO
    FFmpegVideoPlayer *fFmpegVideoPlayer = (FFmpegVideoPlayer *) IFFmepgVideoPlayer;
    ANativeWindow* aNativeWindow = ANativeWindow_fromSurface(env,  surface);


    fFmpegVideoPlayer->play(path, aNativeWindow);


    env->ReleaseStringUTFChars(path_, path);
}
}

