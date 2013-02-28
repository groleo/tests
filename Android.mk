# Build the unit tests.
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := egl-test

LOCAL_MODULE_TAGS := tests
LOCAL_CFLAGS += -DDEBUG -UNDEBUG

LOCAL_SRC_FILES := \
    egl-test.cpp \

LOCAL_SHARED_LIBRARIES := \
	libEGL \
	libGLESv2 \
	libbinder \
	libcutils \
	libgui \
	libmedia \
	libstagefright \
	libstagefright_omx \
	libstagefright_foundation \
	libstlport \
	libui \
	libutils \

LOCAL_STATIC_LIBRARIES := \
	libgtest \
	libgtest_main \

LOCAL_C_INCLUDES := \
    bionic \
    bionic/libstdc++/include \
    external/gtest/include \
    external/stlport/stlport \
	frameworks/av/media/libstagefright \
	frameworks/av/media/libstagefright/include \
	$(TOP)/frameworks/native/include/media/openmax \

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)

LOCAL_MODULE := native-test

LOCAL_MODULE_TAGS := tests
LOCAL_CFLAGS += -DDEBUG -UNDEBUG

LOCAL_SRC_FILES := \
    native-test.cpp \

LOCAL_SHARED_LIBRARIES := \
	libEGL \
	libGLESv2 \
	libbinder \
	libcutils \
	libgui \
	libmedia \
	libstagefright \
	libstagefright_omx \
	libstagefright_foundation \
	libstlport \
	libui \
	libutils \

LOCAL_STATIC_LIBRARIES := \
	libgtest \
	libgtest_main \

LOCAL_C_INCLUDES := \
    bionic \
    bionic/libstdc++/include \
    external/gtest/include \
    external/stlport/stlport \
	frameworks/av/media/libstagefright \
	frameworks/av/media/libstagefright/include \
	$(TOP)/frameworks/native/include/media/openmax \

include $(BUILD_EXECUTABLE)



include $(CLEAR_VARS)

LOCAL_MODULE := mr-test

LOCAL_MODULE_TAGS := tests
LOCAL_CFLAGS += -DDEBUG -UNDEBUG

LOCAL_SRC_FILES := \
    mr-test.cpp \

LOCAL_SHARED_LIBRARIES := \
	libEGL \
	libGLESv2 \
	libbinder \
	libcutils \
	libgui \
	libmedia \
	libstagefright \
	libstagefright_omx \
	libstagefright_foundation \
	libstlport \
	libui \
	libutils \

LOCAL_STATIC_LIBRARIES := \
	libgtest \
	libgtest_main \

LOCAL_C_INCLUDES := \
    bionic \
    bionic/libstdc++/include \
    external/gtest/include \
    external/stlport/stlport \
	frameworks/av/media/libstagefright \
	frameworks/av/media/libstagefright/include \
	$(TOP)/frameworks/native/include/media/openmax \

include $(BUILD_EXECUTABLE)



# Include subdirectory makefiles
# ============================================================

# If we're building with ONE_SHOT_MAKEFILE (mm, mmm), then what the framework
# team really wants is to build the stuff defined by this makefile.
ifeq (,$(ONE_SHOT_MAKEFILE))
include $(call first-makefiles-under,$(LOCAL_PATH))
endif
