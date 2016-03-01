#include <jni.h>
#include <android/native_window_jni.h>
#include "CPlayer.h"

enum {
    OPT_URI = 0,
    OPT_RTSP_PROTOCOL,
    OPT_PACKET_BUFFER_SIZE,
    OPT_IS_FLUSH,
    OPT_IS_MAX_FPS,
    OPT_IS_SKIP_PACKET,
    OPT_IS_LOOP_PLAYING,
    OPT_IS_WINDOW_NATIVE,
    OPT_IS_WINDOW_GLES,
    OPT_IS_VIDEO_QUEUE
};

extern "C" {


jfieldID getJavaPointerToPlayer(JNIEnv *env, jobject obj){
    jclass cls = env->GetObjectClass(obj);
    return env->GetFieldID(cls, "pointerToPlayer", "J");
}

void Java_com_ror13_sysrazplayer_CPlayer_open(JNIEnv *env, jobject thiz, jobject config, jobject surface){
    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env,surface);

    CPlayer* player = (CPlayer*) env->GetLongField(thiz, getJavaPointerToPlayer(env, thiz));
    if(player != NULL) {
        env->SetLongField(thiz, getJavaPointerToPlayer(env, thiz), (jlong) NULL);
        delete player;
    }

    CPlayer::CplayerConfig cplayerConfig;
    jmethodID getValObj = env->GetMethodID(env->GetObjectClass(config), "getValObj", "(I)Ljava/lang/Object;");
    jmethodID getValInt = env->GetMethodID(env->GetObjectClass(config), "getValInt", "(I)I");
    jmethodID getValBool = env->GetMethodID(env->GetObjectClass(config), "getValBool", "(I)Z");

    jobject uri = env->CallObjectMethod(config,getValObj,OPT_URI);
    cplayerConfig.uri = (env)->GetStringUTFChars((jstring)uri, 0);

    jobject rtspProtocol = env->CallObjectMethod(config,getValObj,OPT_RTSP_PROTOCOL);
    cplayerConfig.rtspProtocol = (env)->GetStringUTFChars((jstring)rtspProtocol, 0);



    cplayerConfig.packetBufferSize = (jint) env->CallIntMethod(config,getValInt,OPT_PACKET_BUFFER_SIZE);


    cplayerConfig.isFlush = (jboolean) env->CallBooleanMethod(config,getValBool,OPT_IS_FLUSH);
    cplayerConfig.isMaxFps = (jboolean) env->CallBooleanMethod(config,getValBool,OPT_IS_MAX_FPS);
    cplayerConfig.isSkipPacket = (jboolean) env->CallBooleanMethod(config,getValBool,OPT_IS_SKIP_PACKET);
    cplayerConfig.isLoopPlaying = (jboolean) env->CallBooleanMethod(config,getValBool,OPT_IS_LOOP_PLAYING);
    cplayerConfig.isWindowNative = (jboolean) env->CallBooleanMethod(config,getValBool,OPT_IS_WINDOW_NATIVE);
    cplayerConfig.isWindowGles = (jboolean) env->CallBooleanMethod(config,getValBool,OPT_IS_WINDOW_GLES);
    cplayerConfig.isVideoQueue = (jboolean) env->CallBooleanMethod(config,getValBool,OPT_IS_VIDEO_QUEUE);


    player = new CPlayer();
    player->open(nativeWindow,  &cplayerConfig);
    env->SetLongField(thiz, getJavaPointerToPlayer(env, thiz), (jlong)player);

}

void Java_com_ror13_sysrazplayer_CPlayer_close(JNIEnv *env, jobject thiz){
    CPlayer* player = (CPlayer*) env->GetLongField(thiz, getJavaPointerToPlayer(env, thiz));
    delete player;
    env->SetLongField(thiz, getJavaPointerToPlayer(env, thiz), (jlong)NULL);
}

void Java_com_ror13_sysrazplayer_CPlayer_start(JNIEnv *env, jobject thiz){
    CPlayer* player = (CPlayer*) env->GetLongField(thiz, getJavaPointerToPlayer(env, thiz));
    player->start();
}

void Java_com_ror13_sysrazplayer_CPlayer_stop(JNIEnv *env, jobject thiz){
    CPlayer* player = (CPlayer*) env->GetLongField(thiz, getJavaPointerToPlayer(env, thiz));
    player->stop();
}



jstring Java_com_ror13_sysrazplayer_CFfmpeg_tst( JNIEnv* env, jobject thiz ){
#if defined(__arm__)
    #if defined(__ARM_ARCH_7A__)
      #if defined(__ARM_NEON__)
        #if defined(__ARM_PCS_VFP)
          #define ABI "armeabi-v7a/NEON (hard-float)"
        #else
          #define ABI "armeabi-v7a/NEON"
        #endif
      #else
        #if defined(__ARM_PCS_VFP)
          #define ABI "armeabi-v7a (hard-float)"
        #else
          #define ABI "armeabi-v7a"
        #endif
      #endif
    #else
     #define ABI "armeabi"
    #endif
#elif defined(__i386__)
    #define ABI "x86"
#elif defined(__x86_64__)
    #define ABI "x86_64"
#elif defined(__mips64)  /* mips64el-* toolchain defines __mips__ too */
    #define ABI "mips64"
#elif defined(__mips__)
    #define ABI "mips"
#elif defined(__aarch64__)
    #define ABI "arm64-v8a"
#else
#define ABI "unknown"
#endif

    return (env)->NewStringUTF( "Hello from JNI !  Compiled with ABI " ABI ".");
}

}



