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
    #define GL_BUFFER_ARRAY_SIZE  3
    #define GL_TEXTURE_ARRAY_SIZE  3
    GLuint LoadShader(GLenum shaderType, const char* pSource);

    EGLDisplay  display;
    EGLSurface  surface;
    EGLContext  context;
    EGLConfig   config;
    EGLint      format;
    EGLint      width;
    EGLint      height;

    int32_t mFrameWidth;
    int32_t mFrameHeight;

    GLuint	mVertexShader;
    GLuint	mFragmentShader;

    GLuint  mProgramObject;
    GLuint  mTextureIds[GL_TEXTURE_ARRAY_SIZE];
    GLuint  mBufs[GL_BUFFER_ARRAY_SIZE];

    COLORSPACE mColorspace;

};






#endif //SRPLAYER_CGLESRENDER_H
