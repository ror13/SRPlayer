//
// Created by ror13 on 2/8/16.
//

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


const char* pVertexShaderStrMatrix =
        "uniform mat4 u_MMatrix;					       				\n \
        uniform mat4 u_VMatrix;						    			\n \
        attribute vec4 a_position;							    	\n \
        void main() {									    			\n \
            gl_Position = a_position * u_MMatrix * u_VMatrix ;	\n \
        }													   			\n";

const char* pVertexShaderSimple =
        "attribute vec4 a_position;							    	\n \
        void main() {									    			\n \
            gl_Position = a_position ;								\n \
        }													   			\n";


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

const char* pFragmentShaderColor =
        "precision mediump float;										\n \
        uniform vec4 vColor;											\n \
        void main()													\n \
        {																\n \
             gl_FragColor = vColor;									\n \
        }																\n";

const char* pFragmentShaderSimple =
        "precision mediump float;										\n \
        void main()													\n \
        {																\n \
             gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); 				\n \
        }																\n";

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
void CGlesRender::initialize(COLORSPACE colorspace){
    mColorspace = colorspace;
    mProgramObject		= 0;
    mMirror				= false;
    mDisplayOrientation	= 0;




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
    // Textures
    glGenTextures(2, mTextureIds);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureIds[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mTextureIds[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    LOGD("VBO");

    //VBO
    glGenBuffers(3, mBufs);
    GLfloat vScale = 1.0;
    GLfloat vVertices[] = { -vScale,  vScale, 0.0f, //1.0f,  // Position 0
                            -vScale, -vScale, 0.0f, //1.0f, // Position 1
                            vScale, -vScale, 0.0f, //1.0f, // Position 2
                            vScale,  vScale, 0.0f, //1.0f,  // Position 3
    };

    GLfloat tCoords[] = {0.0f,  0.0f,
                         0.0f,  1.0f,
                         1.0f,  1.0f,
                         1.0f,  0.0f};

    int degree = 0;

    while (mDisplayOrientation > degree) {
        GLfloat temp[2];
        degree += 90;
        temp[0] = tCoords[0]; temp[1] = tCoords[1];
        tCoords[0] = tCoords[2]; tCoords[1] = tCoords[3];
        tCoords[2] = tCoords[4]; tCoords[3] = tCoords[5];
        tCoords[4] = tCoords[6]; tCoords[5] = tCoords[7];
        tCoords[6] = temp[0]; tCoords[7] = temp[1];
    }

    if (mDisplayOrientation == 0 || mDisplayOrientation == 180) {
        if (mMirror == 1){
            GLfloat temp[2];
            LOGD("set mirror is true");
            temp[0] = tCoords[0]; temp[1] = tCoords[2];
            tCoords[0] = tCoords[4]; tCoords[2] = tCoords[6];
            tCoords[4] = temp[0]; tCoords[6] = temp[1];
        }
    } else {
        if (mMirror == 1){
            GLfloat temp[2];
            LOGD("set mirror is true");
            temp[0] = tCoords[1]; temp[1] = tCoords[3];
            tCoords[1] = tCoords[5]; tCoords[3] = tCoords[7];
            tCoords[5] = temp[0]; tCoords[7] = temp[1];
        }
    }

    GLushort indexs[] = { 0, 1, 2, 0, 2, 3 };

    glBindBuffer(GL_ARRAY_BUFFER, mBufs[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, mBufs[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tCoords), tCoords, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBufs[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexs), indexs, GL_STATIC_DRAW);

    LOGD("glesInit() --->");




}

void CGlesRender::draw(void * data, int w, int h){
/*
    LPOPENGLES engine = (LPOPENGLES)handle;
    if (pData == NULL) {
        LOGE("pOffScreen == MNull");
        return;
    }
*/
    //clean
    glClear ( GL_COLOR_BUFFER_BIT );

    //Texture -> GPU
    if (mColorspace == COLOR_NV12 || mColorspace == COLOR_NV21) {
        glBindTexture(GL_TEXTURE_2D, mTextureIds[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

        glBindTexture(GL_TEXTURE_2D, mTextureIds[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, w >> 1, h >> 1, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data + w * h);
    } else if (mColorspace == COLOR_YUYV) {
        glBindTexture(GL_TEXTURE_2D, mTextureIds[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, w, h, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);

        glBindTexture(GL_TEXTURE_2D, mTextureIds[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w >> 1, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    // use shader
    glUseProgram (mProgramObject);

    GLuint textureUniformY = glGetUniformLocation(mProgramObject, "y_texture");
    GLuint textureUniformU = glGetUniformLocation(mProgramObject, "uv_texture");

    glBindTexture(GL_TEXTURE_2D, mTextureIds[0]);
    glUniform1i(textureUniformY, 0);

    glBindTexture(GL_TEXTURE_2D, mTextureIds[1]);
    glUniform1i(textureUniformU, 1);

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
