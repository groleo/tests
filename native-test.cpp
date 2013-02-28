#define LOG_NDEBUG 0
#define DEBUG 1
#undef LOG_TAG
#define LOG_TAG "native-test"

#include <gtest/gtest.h>
#include <binder/ProcessState.h>

#include <ui/GraphicBuffer.h>
#include <gui/SurfaceComposerClient.h>


using namespace android;

sp<ANativeWindow>         gANW;
sp<SurfaceComposerClient> gComposerClient;
sp<SurfaceControl>        gSurfaceControl;


int getSurfaceWidth()  { return 512; }
int getSurfaceHeight() { return 512; }

int getYuvTexWidth()  { return 512; }
int getYuvTexHeight() { return 512; }

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

void setupNativeWindow() {
    status_t err;
    int minUndequeuedBuffers;
    int maxBufferSlack=1;
    int rv;

    rv = native_window_api_connect(gANW.get(), NATIVE_WINDOW_API_CPU);
    assert(rv==0);

    err = native_window_set_buffers_geometry(gANW.get(), getYuvTexWidth(), getYuvTexHeight(), HAL_PIXEL_FORMAT_YV12);
    assert(NO_ERROR == err);

    err = native_window_set_usage(gANW.get(), GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_OFTEN);
    assert(NO_ERROR == err);

    err = gANW.get()->query(gANW.get(), NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS, &minUndequeuedBuffers);
    assert(NO_ERROR == err);

    err = native_window_set_buffer_count(gANW.get(),maxBufferSlack + 1 + minUndequeuedBuffers);
    assert(NO_ERROR == err);
}

void fillYV12Buffer(uint8_t* buf, int w, int h, int stride) {
    const int blockWidth = w > 16 ? w / 16 : 1;
    const int blockHeight = h > 16 ? h / 16 : 1;
    const int yuvTexOffsetY = 0;
    int yuvTexStrideY = stride;
    int yuvTexOffsetV = yuvTexStrideY * h;
    int yuvTexStrideV = (yuvTexStrideY/2 + 0xf) & ~0xf;
    int yuvTexOffsetU = yuvTexOffsetV + yuvTexStrideV * h/2;
    int yuvTexStrideU = yuvTexStrideV;
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            int parityX = (x / blockWidth) & 1;
            int parityY = (y / blockHeight) & 1;
            unsigned char intensity = (parityX ^ parityY) ? 63 : 191;
            buf[yuvTexOffsetY + (y * yuvTexStrideY) + x] = intensity;
            if (x < w / 2 && y < h / 2) {
                buf[yuvTexOffsetU + (y * yuvTexStrideU) + x] = intensity;
                if (x * 2 < w / 2 && y * 2 < h / 2) {
                    buf[yuvTexOffsetV + (y*2 * yuvTexStrideV) + x*2 + 0] =
                    buf[yuvTexOffsetV + (y*2 * yuvTexStrideV) + x*2 + 1] =
                    buf[yuvTexOffsetV + ((y*2+1) * yuvTexStrideV) + x*2 + 0] =
                    buf[yuvTexOffsetV + ((y*2+1) * yuvTexStrideV) + x*2 + 1] =
                        intensity;
                }
            }
        }
    }
}

void oneBufferPassCPU(int a ) {
    ANativeWindowBuffer* anb;
    int fenceFd=-1;
    uint8_t* img = NULL;

    assert(NO_ERROR == native_window_dequeue_buffer_and_wait(gANW.get(), &anb));
    sp<GraphicBuffer> buf(new GraphicBuffer(anb, false));
    buf->lock( GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_OFTEN, (void**)(&img));
    fillYV12Buffer(img, getYuvTexWidth(), getYuvTexHeight(), buf->getStride());
    buf->unlock();
    assert(NO_ERROR == gANW->queueBuffer(gANW.get(), buf->getNativeBuffer(), -1));
}

int main() {
    gANW = createNativeWindow();

    setupNativeWindow();

    for(int nFramesCount=0;nFramesCount< 50; ++nFramesCount)
        oneBufferPassCPU(nFramesCount);

    sleep(10);

    if (gComposerClient != NULL) {
        gComposerClient->dispose();
    }
}
