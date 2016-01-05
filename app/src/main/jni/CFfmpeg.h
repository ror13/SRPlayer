//
// Created by ror13 on 13.12.15.
//

#ifndef SRPLAYER_CFFMPEG_H
#define SRPLAYER_CFFMPEG_H
#include <jni.h>
#include <string.h>
#include <android/log.h>
//#if defined(__arm__)
//#include <android/window.h>
//#include <android/window_jni.h>
//#else
#include <android/native_window.h>
#include <android/native_window_jni.h>
//#endif
#include <android/rect.h>
extern "C" {
jstring
Java_com_ror13_sysrazplayer_CFfmpeg_tst(JNIEnv *env, jobject thiz);
void
Java_com_ror13_sysrazplayer_CFfmpeg_play(JNIEnv *env, jobject thiz, jstring path, jobject surface);
}
#endif //SRPLAYER_CFFMPEG_H
