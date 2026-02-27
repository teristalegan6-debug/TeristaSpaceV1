LOCAL_PATH := $(call my-dir)

# Terista Native Hook Library
include $(CLEAR_VARS)
LOCAL_MODULE := teristanative

LOCAL_SRC_FILES := \
    terista_native.cpp \
    elf_utils.cpp \
    inline_hook.cpp \
    binder_hook.cpp \
    symbol_resolver.cpp \
    jni_bridge.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/include

LOCAL_CPPFLAGS := -std=c++17 -fexceptions -frtti -DANDROID
LOCAL_CFLAGS := -DANDROID -ffunction-sections -fdata-sections

LOCAL_LDLIBS := -llog -ldl -lz
LOCAL_LDFLAGS := -Wl,--gc-sections

include $(BUILD_SHARED_LIBRARY)