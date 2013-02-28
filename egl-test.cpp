#define LOG_NDEBUG 0
#define DEBUG 1
#undef LOG_TAG
#define LOG_TAG "egl-test"

#include <gtest/gtest.h>

#include <ui/GraphicBuffer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/CpuConsumer.h>
#include <gui/SurfaceTexture.h>
#include <gui/SurfaceTextureClient.h>
#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>

#include <unistd.h>
#include <fcntl.h>
////
#include <media/stagefright/SurfaceMediaSource.h>
#include <media/mediarecorder.h>

#include <binder/ProcessState.h>

using namespace android;

sp<SurfaceMediaSource> mSMS;
sp<SurfaceTextureClient> mSTC;
sp<ANativeWindow> mANW;
sp<SurfaceComposerClient> mComposerClient;
sp<SurfaceControl> mSurfaceControl;
sp<MediaRecorder> mREC;

int getSurfaceWidth()  { return 512; }
int getSurfaceHeight() { return 512; }

int getYuvTexWidth()  { return 512; }
int getYuvTexHeight() { return 512; }

EGLint const* getConfigAttribs() {
    ALOGV("SurfaceMediaSourceGLTest getConfigAttribs");
    static EGLint sDefaultConfigAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RECORDABLE_ANDROID, EGL_TRUE,
        EGL_NONE };

    return sDefaultConfigAttribs;
}

EGLint const* getContextAttribs() {
    static EGLint sDefaultContextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE };

    return sDefaultContextAttribs;
}

class Egl {
protected:
    EGLConfig  mGlConfig;
    EGLDisplay mEglDisplay;
    EGLContext mEglContext;
    EGLSurface mEglSurface;
public:
    Egl() {}
    void setupDisplay() {
        // setupDisplay
        mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        assert(EGL_SUCCESS == eglGetError());
        assert(EGL_NO_DISPLAY != mEglDisplay);
    }
    void init() {
        // Tie EGL to the display
        EGLint majorVersion;
        EGLint minorVersion;
        int rv;

        rv = eglInitialize(mEglDisplay, &majorVersion, &minorVersion);
        assert(rv == true);
        assert(EGL_SUCCESS == eglGetError());
    }
    void chooseConfig() {
        //Get configs
        EGLint numConfigs = 0;
        int rv;

        rv = eglChooseConfig(mEglDisplay, getConfigAttribs(), &mGlConfig, 1, &numConfigs);
        assert(rv==true);
        assert(EGL_SUCCESS == eglGetError());
    }
    void createContext() {
        ///////
        mEglContext = eglCreateContext(mEglDisplay, mGlConfig, EGL_NO_CONTEXT, getContextAttribs());
        assert(EGL_SUCCESS == eglGetError());
        assert(EGL_NO_CONTEXT != mEglContext);
    }
    void makeCurrent() {
        int rv;
        rv = eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext);
        assert(rv == true);
        assert(EGL_SUCCESS == eglGetError());
    }
    void setupViewport() {
        EGLint w, h;
        int rv;

        rv = eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, &w);
        assert(rv == true);
        assert(EGL_SUCCESS == eglGetError());

        rv = eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, &h);
        assert(rv == true);
        assert(EGL_SUCCESS == eglGetError());

        glViewport(0, 0, w, h);
        assert(GLenum(GL_NO_ERROR) == glGetError());
    }
    void destroyWindowSurface() {
        int rv;
        if (mEglSurface != EGL_NO_SURFACE) {
            rv=eglDestroySurface(mEglDisplay, mEglSurface);
            assert(rv == true);
            mEglSurface = EGL_NO_SURFACE;
        }
    }
    void createWindowSurface(ANativeWindow* nw){
        mEglSurface = eglCreateWindowSurface(mEglDisplay, mGlConfig, nw, NULL);
        assert(EGL_SUCCESS == eglGetError());
        assert(EGL_NO_SURFACE != mEglSurface);
    }
    void swapBuffers() {
        eglSwapBuffers(mEglDisplay, mEglSurface);
        //sleep(1);
        assert(EGL_SUCCESS == eglGetError());
    }
    void shutdown() {
        int rv;
        rv = eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        assert(rv == true);
        assert(EGL_SUCCESS == eglGetError());

        if (mEglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(mEglDisplay, mEglContext);
        }
        if (mEglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(mEglDisplay, mEglSurface);
            mEglSurface = EGL_NO_SURFACE;
        }
        if (mEglDisplay != EGL_NO_DISPLAY) {
            eglTerminate(mEglDisplay);
        }
        assert(EGL_SUCCESS == eglGetError());
    }
};



static void loadShader(GLenum shaderType, const char* pSource,
        GLuint* outShader) {
    GLuint shader = glCreateShader(shaderType);
    ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
    ASSERT_TRUE(shader != 0);

    if (shader) {
        printf("SHADER != NULL\n");
        glShaderSource(shader, 1, &pSource, NULL);
        ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
        glCompileShader(shader);
        ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    printf("Shader compile log:\n%s\n", buf);
                    free(buf);
                    FAIL();
                }
            } else {
                char* buf = (char*) malloc(0x1000);
                if (buf) {
                    glGetShaderInfoLog(shader, 0x1000, NULL, buf);
                    printf("Shader compile log:\n%s\n", buf);
                    free(buf);
                    FAIL();
                }
            }
            glDeleteShader(shader);
            shader = 0;
        }
    }
    else
        printf("SHADER === NULL\n");
    *outShader = shader;
}


static void createProgram(const char* pVertexSource,
        const char* pFragmentSource, GLuint* outPgm) {
    GLuint vertexShader, fragmentShader;
    {
        SCOPED_TRACE("compiling vertex shader");
        ASSERT_NO_FATAL_FAILURE(loadShader(GL_VERTEX_SHADER, pVertexSource, &vertexShader));
    }
    {
        SCOPED_TRACE("compiling fragment shader");
        ASSERT_NO_FATAL_FAILURE(loadShader(GL_FRAGMENT_SHADER, pFragmentSource, &fragmentShader));
    }

    GLuint program = glCreateProgram();
    ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
    if (program) {
        glAttachShader(program, vertexShader);
        ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
        glAttachShader(program, fragmentShader);
        ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    printf("Program link log:\n%s\n", buf);
                    free(buf);
                    FAIL();
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    ASSERT_TRUE(program != 0);
    *outPgm = program;
}


class TextureRenderer: public RefBase {
    public:
        TextureRenderer(GLuint texName, const sp<SurfaceTexture>& st):
            mTexName(texName),
            mST(st) {
            }

        void SetUp() {
            const char vsrc1[] =
                "attribute vec4 vPosition;\n"
                "varying vec2 texCoords;\n"
                "uniform mat4 texMatrix;\n"
                "void main() {\n"
                "  vec2 vTexCoords = 0.5 * (vPosition.xy + vec2(1.0, 1.0));\n"
                "  texCoords = (texMatrix * vec4(vTexCoords, 0.0, 1.0)).xy;\n"
                "  gl_Position = vPosition;\n"
                "}\n";
            const char fsrc1[] =
                "#extension GL_OES_EGL_image_external : require\n"
                "precision mediump float;\n"
                "uniform samplerExternalOES texSampler;\n"
                "varying vec2 texCoords;\n"
                "void main() {\n"
                "  gl_FragColor = texture2D(texSampler, texCoords);\n"
                "}\n";

            const char vsrc[] =
                "void main() {\n"
                "  gl_Position = vPosition;\n"
                "}\n";
            const char fsrc[] =
                "void main() {\n"
                "gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
                "}\n";

            {
                ASSERT_NO_FATAL_FAILURE(createProgram(vsrc, fsrc, &mPgm));
            }

            mPositionHandle = glGetAttribLocation(mPgm, "vPosition");
            ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
            ASSERT_NE(-1, mPositionHandle);
            mTexSamplerHandle = glGetUniformLocation(mPgm, "texSampler");
            ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
            ASSERT_NE(-1, mTexSamplerHandle);
            mTexMatrixHandle = glGetUniformLocation(mPgm, "texMatrix");
            ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
            ASSERT_NE(-1, mTexMatrixHandle);
        }

        // drawTexture draws the SurfaceTexture over the entire GL viewport.
        void drawTexture() {
            static const GLfloat triangleVertices[] = {
                -1.0f, 1.0f,
                -1.0f, -1.0f,
                1.0f, -1.0f,
                1.0f, 1.0f,
            };

            glVertexAttribPointer(mPositionHandle, 2, GL_FLOAT, GL_FALSE, 0,
                    triangleVertices);
            ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
            glEnableVertexAttribArray(mPositionHandle);
            ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());

            glUseProgram(mPgm);
            glUniform1i(mTexSamplerHandle, 0);
            ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTexName);
            ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());

            // XXX: These calls are not needed for GL_TEXTURE_EXTERNAL_OES as
            // they're setting the defautls for that target, but when hacking
            // things to use GL_TEXTURE_2D they are needed to achieve the same
            // behavior.
            glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
            glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
            glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
            glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());

            GLfloat texMatrix[16];
            mST->getTransformMatrix(texMatrix);
            glUniformMatrix4fv(mTexMatrixHandle, 1, GL_FALSE, texMatrix);

            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            ASSERT_EQ(GLenum(GL_NO_ERROR), glGetError());
        }

        GLuint mTexName;
        sp<SurfaceTexture> mST;
        GLuint mPgm;
        GLint mPositionHandle;
        GLint mTexSamplerHandle;
        GLint mTexMatrixHandle;
};




sp<TextureRenderer> mTextureRenderer;
sp<SurfaceTexture> mST;


Egl gEgl;

/////***********************************
void oneBufferPassGPU(int num) {
    int d = num % 50;
    float f = 0.2f; // 0.1f * d;

    glClearColor(0.3, 0.0, 0.5, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_SCISSOR_TEST);
    glScissor(4 + d, 4 + d, 14, 14);
    glClearColor(1.0 - f, f, f, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glScissor(24 + d, 48 + d, 14, 14);
    glClearColor(f, 1.0 - f, f, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glScissor(37 + d, 17 + d, 14, 14);
    glClearColor(f, f, 1.0 - f, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // The following call dequeues and queues the buffer
    gEgl.swapBuffers();
    glDisable(GL_SCISSOR_TEST);
}

void setUpEGLSurfaceFromMediaRecorder(sp<MediaRecorder>& mr) {

    sp<ISurfaceTexture> iST = mr->querySurfaceMediaSourceFromMediaServer();
    assert(iST !=(void*)NULL);

    mSTC = new SurfaceTextureClient(iST);
    mANW = mSTC;
    assert(mANW != NULL);

    gEgl.destroyWindowSurface();
    gEgl.createWindowSurface(mANW.get());
    gEgl.makeCurrent();
}


void setupSurfaces() {
#if 0
    if (type == USE_CPU) {
        int TEX_ID=123;
        mST = new SurfaceTexture(TEX_ID);
        mSTC = new SurfaceTextureClient(mST);
        mANW = mSTC;

        mTextureRenderer = new TextureRenderer(TEX_ID, mST);
        mTextureRenderer->SetUp();
        mFW = new FrameWaiter;
        mST->setFrameAvailableListener(mFW);
    }else if(type==USE_GPU) {
#endif
    mSMS = new SurfaceMediaSource(getYuvTexWidth(), getYuvTexHeight());
    mSTC = new SurfaceTextureClient(static_cast<sp<ISurfaceTexture> >( mSMS->getBufferQueue()));
    mANW = mSTC;
#if 0
    }
#endif
}

sp<ANativeWindow> createNativeWindow() {
    mComposerClient = new SurfaceComposerClient;
    assert(NO_ERROR == mComposerClient->initCheck());

    mSurfaceControl = mComposerClient->createSurface(String8("Test Surface"),
            getSurfaceWidth(), getSurfaceHeight(),
            PIXEL_FORMAT_RGB_888, 0);
    assert(mSurfaceControl != NULL);
    assert(mSurfaceControl->isValid());

    SurfaceComposerClient::openGlobalTransaction();
    assert(NO_ERROR == mSurfaceControl->setLayer(0x7FFFFFFF));

    //mSurfaceControl->setPosition(1,1);
    assert(NO_ERROR == mSurfaceControl->show());
    SurfaceComposerClient::closeGlobalTransaction();
    return mSurfaceControl->getSurface();
}

void createEglWindowSurface(bool display) {
    sp<ANativeWindow> window;
    if (display){
        window = createNativeWindow();
    }else
    {
        ALOGV("No actual display. Choosing EGLSurface based on SurfaceMediaSource");
        sp<ISurfaceTexture> sms = (new SurfaceMediaSource( getSurfaceWidth(), getSurfaceHeight()))->getBufferQueue();
        sp<SurfaceTextureClient> stc = new SurfaceTextureClient(sms);
        window = stc;
    }
    mANW = window ;
    gEgl.createWindowSurface(window.get());
}


void renderFrames(int numFrames) {
    for (int nFramesCount=0;nFramesCount<numFrames;nFramesCount++)
        oneBufferPassGPU(nFramesCount);
}


class Media {
protected:
    int fd;
    int openMP4(const char *fileName) {
        int fd = open(fileName, O_RDWR | O_CREAT, 0744);
        if (fd < 0) {
            ALOGE("ERROR: Could not open the the file %s, fd = %d !!", fileName, fd);
        }
        assert(fd >= 0);
        return fd;
    }
    // Set up the MediaRecorder which runs in the same process as mediaserver
    sp<MediaRecorder> setUpMediaRecorder(int fd, int videoSource,
            int outputFormat, int videoEncoder, int width, int height, int fps) {
        sp<MediaRecorder> mr = new MediaRecorder();

        assert(0 == mr->setVideoSource(videoSource));
        assert(0 == mr->setOutputFormat(outputFormat));
        assert(0 == mr->setVideoEncoder(videoEncoder));
        assert(0 == mr->setOutputFile(fd, 0, 0));
        assert(0 == mr->setVideoSize(width, height));
        assert(0 == mr->setVideoFrameRate(fps));
        assert(0 == mr->prepare());
        ALOGI("Starting MediaRecorder...");
        assert((status_t)OK == mr->start());
        return mr;
    }

public:
    void setup() {
        fd = openMP4("/sdcard/1.mp4");
        assert(fd >= 0);
        mREC = setUpMediaRecorder(fd, VIDEO_SOURCE_GRALLOC_BUFFER, OUTPUT_FORMAT_MPEG_4, VIDEO_ENCODER_H264, getYuvTexWidth(), getYuvTexHeight(), 30);
    }
    void shutdown() {
        ALOGI("Stopping MediaRecorder...");
        assert((status_t)OK == mREC->stop());
        mREC.clear();
        close(fd);
    }
};

void shutdownSurfaces() {
    mSMS.clear();
    mSTC.clear();
    mANW.clear();
}


Media m;

int main() {
    int rv;
    android::ProcessState::self()->startThreadPool();

    gEgl.setupDisplay();
    setupSurfaces();
    gEgl.init();
    gEgl.chooseConfig();
    createEglWindowSurface(true);
    gEgl.createContext();
    gEgl.makeCurrent();
    gEgl.setupViewport();
    m.setup();
    // get a reference to SurfaceMediaSource living in
    // MediaServer that is created by StagefrightRecorder
    setUpEGLSurfaceFromMediaRecorder(mREC);
    renderFrames(50);

    gEgl.shutdown();
    m.shutdown();
    shutdownSurfaces();

    if (mComposerClient != NULL) {
        mComposerClient->dispose();
    }
}
