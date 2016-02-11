//
// Created by ror13 on 2/8/16.
//
// https://android.googlesource.com/platform/frameworks/av/+/master/media/libstagefright/tests/SurfaceMediaSource_test.cpp
// https://gist.github.com/lifuzu/9958640
#include "CGlesRender.h"
#include <android/log.h>
#define LOGD(...) __android_log_print(ANDROID_LOG_INFO, "LOG_TAG", __VA_ARGS__)
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, "LOG_TAG", __VA_ARGS__)
#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, "LOG_TAG", __VA_ARGS__)


const char* pVertexShaderStr =
        "attribute vec4 a_position;   								\n \
        attribute vec2 a_texCoord;   								\n \
        varying highp vec2 v_texCoord; 								\n \
        void main()                  								\n \
        {                            								\n \
            gl_Position = a_position; 								\n \
            v_texCoord = a_texCoord;  								\n \
        }                            								\n";

const char* pFragmentShaderYUYV =
        "precision highp float;										\n \
        uniform sampler2D y_texture;									\n \
        uniform sampler2D uv_texture;								\n \
        varying highp vec2 v_texCoord;								\n \
        void main()													\n \
        {																\n \
            mediump vec3 yuv;											\n \
            highp vec3 rgb; 											\n \
            yuv.x = texture2D(y_texture, v_texCoord).r;  			\n \
            yuv.y = texture2D(uv_texture, v_texCoord).g-0.5;		\n \
            yuv.z = texture2D(uv_texture, v_texCoord).a-0.5;		\n \
            rgb = mat3(      1,       1,       1,					\n \
                      0, -0.344, 1.770,								\n \
                      1.403, -0.714,       0) * yuv;				\n \
            gl_FragColor = vec4(rgb, 1);								\n \
        }																\n";

const char* pFragmentShaderNV21 =
        "precision highp float;										\n \
        uniform sampler2D y_texture;									\n \
        uniform sampler2D uv_texture;								\n \
        varying highp vec2 v_texCoord;								\n \
        void main()													\n \
        {			 													\n \
            mediump vec3 yuv;											\n \
            highp vec3 rgb; 											\n \
            yuv.x = texture2D(y_texture, v_texCoord).r;  			\n \
            yuv.y = texture2D(uv_texture, v_texCoord).a-0.5;		\n \
            yuv.z = texture2D(uv_texture, v_texCoord).r-0.5;		\n \
            rgb = mat3(      1,       1,       1,					\n \
                      0, -0.344, 1.770,								\n \
                      1.403, -0.714,       0) * yuv;				\n \
            gl_FragColor = vec4(rgb, 1);								\n \
        }																\n";

const char* pFragmentShaderNV12 =
        "precision highp float; 										\n	\
        uniform sampler2D y_texture;									\n \
        uniform sampler2D uv_texture;								\n \
        varying highp vec2 v_texCoord;								\n \
        void main()													\n \
        {																\n \
            mediump vec3 yuv;											\n \
            highp vec3 rgb; 											\n \
            yuv.x = texture2D(y_texture, v_texCoord).r;  			\n \
            yuv.y = texture2D(uv_texture, v_texCoord).r-0.5;		\n \
            yuv.z = texture2D(uv_texture, v_texCoord).a-0.5;		\n \
            rgb = mat3(      1,       1,       1,					\n \
                      0, -0.344, 1.770,								\n \
                      1.403, -0.714,       0) * yuv;				\n \
            gl_FragColor = vec4(rgb, 1);								\n \
        }																\n";

const char* pFragmentShaderYUV420P =
                        "precision highp float; \n"
                        "varying highp vec2 v_texCoord;\n"
                        "uniform sampler2D y_texture;\n"
                        "uniform sampler2D u_texture;\n"
                        "uniform sampler2D v_texture;\n"
                        "void main() {\n"
                        "    highp float y = texture2D(y_texture, v_texCoord).r;\n"
                        "    highp float u = texture2D(u_texture, v_texCoord).r - 0.5;\n"
                        "    highp float v = texture2D(v_texture, v_texCoord).r - 0.5;\n"
                        "    highp float r = clamp(y + 1.596 * v, 0.0, 1.0);\n"
                        "    highp float g = clamp(y - 0.391 * u - 0.813 * v, 0.0, 1.0);\n"
                        "    highp float b = clamp(y + 2.018 * u, 0.0, 1.0);\n"
                        "    gl_FragColor = vec4(r,g,b,1.0);\n"
                        "}\n";

CGlesRender::CGlesRender(ANativeWindow * nativeWindow){
    // INITIALIZE EGL

    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };

    const EGLint attribsContext[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };
    LOG_INFO("Initializing context");

    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        LOG_ERROR("eglGetDisplay() returned error %d", eglGetError());
        return;
    }
    if (!eglInitialize(display, 0, 0)) {
        LOG_ERROR("eglInitialize() returned error %d", eglGetError());
        return;
    }

    if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs)) {
        LOG_ERROR("eglChooseConfig() returned error %d", eglGetError());
        return;
    }

    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
        LOG_ERROR("eglGetConfigAttrib() returned error %d", eglGetError());
        return;
    }

    ANativeWindow_setBuffersGeometry(nativeWindow, 0, 0, format);
    LOG_ERROR("ANativeWindow_setBuffersGeometry(nativeWindow, 0, 0, %d); ", format);

    if (!(surface = eglCreateWindowSurface(display, config, nativeWindow, 0))) {
        LOG_ERROR("eglCreateWindowSurface() returned error %d", eglGetError());
        return;
    }

    if (!(context = eglCreateContext(display, config, 0, attribsContext))) {
        LOG_ERROR("eglCreateContext() returned error %d", eglGetError());
        return;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOG_ERROR("eglMakeCurrent() returned error %d", eglGetError());
        return;
    }

    if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
        !eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
        LOG_ERROR("eglQuerySurface() returned error %d", eglGetError());
        return;
    }
}
void CGlesRender::initialize(COLORSPACE colorspace, int frameWidth, int frameHeight){
    mColorspace = colorspace;
    mFrameHeight = frameHeight;
    mFrameWidth = frameWidth;

    // initialize GLES image ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    GLuint	vertexShader;
    GLuint	fragmentShader;
    GLint	linked;

    vertexShader = LoadShader(GL_VERTEX_SHADER, pVertexShaderStr);
    switch (mColorspace){
        case COLOR_NV21: fragmentShader = LoadShader(GL_FRAGMENT_SHADER, pFragmentShaderNV21);
            break;
        case COLOR_NV12: fragmentShader = LoadShader(GL_FRAGMENT_SHADER, pFragmentShaderNV12);
            break;
        case COLOR_YUYV: fragmentShader = LoadShader(GL_FRAGMENT_SHADER, pFragmentShaderYUYV);
            break;
        case COLOR_YUV420P: fragmentShader = LoadShader(GL_FRAGMENT_SHADER, pFragmentShaderYUV420P);
            break;
        default: LOG_ERROR("create fragmentShader failed");


    }



    mProgramObject = glCreateProgram();
    if (0 == mProgramObject) {
        LOG_ERROR("create programObject failed");
        return;
    }

    LOGD("glAttachShader");

    glAttachShader(mProgramObject, vertexShader);
    glAttachShader(mProgramObject, fragmentShader);

    LOGD("glBindAttribLocation");
    glBindAttribLocation(mProgramObject, 0, "a_position");
    glBindAttribLocation(mProgramObject, 1, "a_texCoord");

    glLinkProgram ( mProgramObject );

    LOGD("glLinkProgram");

    glGetProgramiv(mProgramObject, GL_LINK_STATUS, &linked);
    if (0 == linked) {
        GLint	infoLen = 0;
        LOG_ERROR("link failed");
        glGetProgramiv( mProgramObject, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1) {
            char* infoLog = (char*)malloc(sizeof(char) * infoLen);

            glGetProgramInfoLog(mProgramObject, infoLen, NULL, infoLog);
            LOG_ERROR( "Error linking program: %s", infoLog);

            free(infoLog);
            infoLog = NULL;
        }

        glDeleteProgram(mProgramObject);
        return;
    }

    //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //glEnable(GL_TEXTURE_2D);

    LOGD("glGenTextures");
    int numTextures = 2;
    if(mColorspace == COLOR_YUV420P){
        numTextures = 3;
    }
    // Textures
    glGenTextures(numTextures, mTextureIds);
    for(int i = 0; i < numTextures; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, mTextureIds[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }



    LOGD("VBO");

    //VBO
    glGenBuffers(3, mBufs);

    GLfloat vVertices[] = { -1.0,  1.0, 0.0f, //1.0f,  // Position 0
                            -1.0, -1.0, 0.0f, //1.0f, // Position 1
                            1.0, -1.0, 0.0f, //1.0f, // Position 2
                            1.0,  1.0, 0.0f, //1.0f,  // Position 3
    };

    GLfloat tCoords[] = {0.0f,  0.0f,
                         0.0f,  1.0f,
                         1.0f,  1.0f,
                         1.0f,  0.0f};


    GLushort indexs[] = { 0, 1, 2, 0, 2, 3 };

    glBindBuffer(GL_ARRAY_BUFFER, mBufs[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, mBufs[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tCoords), tCoords, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBufs[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexs), indexs, GL_STATIC_DRAW);

    LOGD("glesInit() --->");




}

void CGlesRender::draw(void ** data){


    glClear ( GL_COLOR_BUFFER_BIT );
    glUseProgram (mProgramObject);

    switch (mColorspace){
        case COLOR_YUV420P:
            glBindTexture(GL_TEXTURE_2D, mTextureIds[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mFrameWidth, mFrameHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data[0]);
            glUniform1i(glGetUniformLocation(mProgramObject, "y_texture"), 0);
            glBindTexture(GL_TEXTURE_2D, mTextureIds[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mFrameWidth /2, mFrameHeight/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data[1]);
            glUniform1i(glGetUniformLocation(mProgramObject, "u_texture"), 1);
            glBindTexture(GL_TEXTURE_2D, mTextureIds[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mFrameWidth/2 , mFrameHeight /2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data[2]);
            glUniform1i(glGetUniformLocation(mProgramObject, "v_texture"), 2);
            break;
        case COLOR_NV12:
        case COLOR_NV21:
            glBindTexture(GL_TEXTURE_2D, mTextureIds[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mFrameWidth, mFrameHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data[0]);
            glUniform1i(glGetUniformLocation(mProgramObject, "y_texture"), 0);
            glBindTexture(GL_TEXTURE_2D, mTextureIds[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, mFrameWidth >> 1, mFrameHeight >> 1, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data[1]);
            glUniform1i(glGetUniformLocation(mProgramObject, "uv_texture"), 1);
            break;
        case COLOR_YUYV:
            glBindTexture(GL_TEXTURE_2D, mTextureIds[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, mFrameWidth, mFrameHeight, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data[0]);
            glUniform1i(glGetUniformLocation(mProgramObject, "y_texture"), 0);
            glBindTexture(GL_TEXTURE_2D, mTextureIds[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mFrameWidth >> 1, mFrameHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data[1]);
            glUniform1i(glGetUniformLocation(mProgramObject, "uv_texture"), 1);
            break;
    }

    glBindBuffer(GL_ARRAY_BUFFER, mBufs[0]);
    glVertexAttribPointer ( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0 );
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, mBufs[1]);
    glVertexAttribPointer ( 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0 );
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBufs[2]);
    glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0 );
}

void CGlesRender::swap(){
    if (!eglSwapBuffers(display, surface)) {
        LOG_ERROR("eglSwapBuffers() returned error %d", 0);
    }
}
void CGlesRender::clear(){

}

GLuint CGlesRender::LoadShader(GLenum shaderType, const char* pSource)
{
    GLuint shader = 0;
    shader = glCreateShader(shaderType);
    LOGD("glGetShaderiv called  shader = %d GL_INVALID_ENUM = %d GL_INVALID_OPERATION = %d", shader, GL_INVALID_ENUM, GL_INVALID_OPERATION);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 1;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        LOGD( "glGetShaderiv called compiled = %d, shader = %d", compiled, shader);
        if (!compiled)
        {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen)
            {
                char* buf = (char*) malloc(infoLen);
                if (buf)
                {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOG_ERROR("Could not compile shader %d: %s",
                         shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
            return 0;
        }
    }
    return shader;
}


CGlesRender::~CGlesRender(){
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);

    display = EGL_NO_DISPLAY;
    surface = EGL_NO_SURFACE;
    context = EGL_NO_CONTEXT;
    width = 0;
    height = 0;
}

