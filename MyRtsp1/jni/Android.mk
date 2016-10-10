LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := x264
LOCAL_SRC_FILES := libx264.so
include $(PREBUILT_SHARED_LIBRARY) 

include $(CLEAR_VARS)
LOCAL_MODULE := live555
LOCAL_SRC_FILES := liblive555.so
include $(PREBUILT_SHARED_LIBRARY) 

include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
     $(LOCAL_PATH)/x264 \
     $(LOCAL_PATH)/live555 \
     $(LOCAL_PATH)/live555/BasicUsageEnvironment/include \
     $(LOCAL_PATH)/live555/liveMedia/include \
     $(LOCAL_PATH)/live555/groupsock/include \
     $(LOCAL_PATH)/live555/UsageEnvironment/include
     
LOCAL_SHARED_LIBRARIES := x264 live555

LOCAL_MODULE := rtspclient
LOCAL_SRC_FILES := testRTSPClient0.cpp
LOCAL_LDLIBS    += -llog -lz

include $(BUILD_SHARED_LIBRARY)
