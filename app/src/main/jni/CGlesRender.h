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
    COLOR_YUYV};
class CGlesRender {
public:
    CGlesRender(ANativeWindow * nativeWindow);
    void initialize(COLORSPACE colorspace);
    void draw(void * data, int w, int h);
    void swap();
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


    GLuint  mProgramObject;
    GLuint  mTextureIds[2];
    GLuint  mBufs[3];

    int     mMirror;
    int     mDisplayOrientation;

    COLORSPACE mColorspace;

};






#endif //SRPLAYER_CGLESRENDER_H
