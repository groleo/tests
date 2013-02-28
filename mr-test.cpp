#define LOG_NDEBUG 0
#define DEBUG 1
#undef LOG_TAG
#define LOG_TAG "mr-test"

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

sp<ANativeWindow>         gANW;
sp<SurfaceComposerClient> gComposerClient;
sp<SurfaceControl>        gSurfaceControl;
sp<MediaRecorder> mREC;

int getSurfaceWidth()  { return 640; }
int getSurfaceHeight() { return 480; }

sp<ANativeWindow> createNativeWindow() {
    gComposerClient = new SurfaceComposerClient;
    assert(NO_ERROR == gComposerClient->initCheck());

    gSurfaceControl = gComposerClient->createSurface(String8("Test Surface"),
            getSurfaceWidth(), getSurfaceHeight(),
            PIXEL_FORMAT_RGB_888, 0);
    assert(gSurfaceControl != NULL);
    assert(gSurfaceControl->isValid());

    SurfaceComposerClient::openGlobalTransaction();
    assert(NO_ERROR == gSurfaceControl->setLayer(0x7FFFFFFF));

    assert(NO_ERROR == gSurfaceControl->show());
    SurfaceComposerClient::closeGlobalTransaction();
    return gSurfaceControl->getSurface();
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

        mr->setPreviewSurface(gSurfaceControl->getSurface());

        assert(0 == mr->prepare());
        ALOGI("Starting MediaRecorder...");
        assert((status_t)OK == mr->start());
        ALOGI("MediaRecorder started");
        return mr;
    }

public:
    void setup() {
        fd = openMP4("/sdcard/mr.mp4");
        assert(fd >= 0);
        mREC = setUpMediaRecorder(fd, VIDEO_SOURCE_CAMERA, OUTPUT_FORMAT_MPEG_4, VIDEO_ENCODER_H264, getSurfaceWidth(), getSurfaceHeight(), 30);
    }
    void shutdown() {
        ALOGI("Stopping MediaRecorder...");
        sleep(5);
        assert((status_t)OK == mREC->stop());
        ALOGI("MediaRecorder stopped");
        mREC.clear();
        close(fd);
    }
};

int main() {
    int rv;
    android::ProcessState::self()->startThreadPool();
    gANW = createNativeWindow();

    Media m;
    m.setup();
    m.shutdown();
}
