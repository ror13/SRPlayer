//
// Created by ror13 on 2/8/16.
//

#ifndef SRPLAYER_CGLESRENDER_H
#define SRPLAYER_CGLESRENDER_H
#include <jni.h>

#include <stdio.h>
#include <string.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
enum COLORSPACE{COLOR_NV12,
    COLOR_NV21,
    COLOR_YUV420P,
    COLOR_YUYV};
class CGlesRender {
public:
    CGlesRender(ANativeWindow * nativeWindow);
    void initialize(COLORSPACE colorspace, int frameWidth, int frameHeight);
    void draw(void ** data);
    void swap();
    void clear();
    ~CGlesRender();
protected:
    GLuint LoadShader(GLenum shaderType, const char* pSource);

    EGLDisplay  display;
    EGLSurface  surface;
    EGLContext  context;
    EGLConfig   config;
    EGLint      numConfigs;
    EGLint      format;
    EGLint      width;
    EGLint      height;

    int32_t mFrameWidth;
    int32_t mFrameHeight;


    GLuint  mProgramObject;
    GLuint  mTextureIds[3];
    GLuint  mBufs[3];

    COLORSPACE mColorspace;

};






#endif //SRPLAYER_CGLESRENDER_H
