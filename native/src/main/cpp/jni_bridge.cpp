#include "include/terista_native.h"
#include <android/log.h>
#include <jni.h>

#define LOG_TAG "JniBridge"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

/**
 * JNI bridge implementation for native hooks and utilities
 */

// Additional JNI functions beyond what's already in terista_native.cpp
extern "C" {

JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_loadLibrary(JNIEnv* env, jobject thiz, jstring path) {
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    bool result = TeristaNative::getInstance()->loadLibrary(pathStr);
    env->ReleaseStringUTFChars(path, pathStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_createVirtualProcess(JNIEnv* env, jobject thiz, jstring packageName, jint userId) {
    const char* packageStr = env->GetStringUTFChars(packageName, nullptr);
    bool result = TeristaNative::getInstance()->createVirtualProcess(packageStr, userId);
    env->ReleaseStringUTFChars(packageName, packageStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_killVirtualProcess(JNIEnv* env, jobject thiz, jint pid) {
    return TeristaNative::getInstance()->killVirtualProcess(pid) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_protectMemory(JNIEnv* env, jobject thiz, jlong addr, jlong size, jint prot) {
    return TeristaNative::getInstance()->protectMemory((void*)addr, (size_t)size, prot) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL Java_com_terista_space_native_NativeBridge_allocateMemory(JNIEnv* env, jobject thiz, jlong size) {
    return (jlong)TeristaNative::getInstance()->allocateMemory((size_t)size);
}

JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_freeMemory(JNIEnv* env, jobject thiz, jlong addr, jlong size) {
    return TeristaNative::getInstance()->freeMemory((void*)addr, (size_t)size) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_setBinderFilter(JNIEnv* env, jobject thiz, jstring serviceName, jboolean allow) {
    const char* serviceStr = env->GetStringUTFChars(serviceName, nullptr);
    bool result = TeristaNative::getInstance()->setBinderFilter(serviceStr, allow == JNI_TRUE);
    env->ReleaseStringUTFChars(serviceName, serviceStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_com_terista_space_native_NativeBridge_unhookBinder(JNIEnv* env, jobject thiz) {
    TeristaNative::getInstance()->unhookBinder();
}

// JNI method registration
static const JNINativeMethod methods[] = {
    {"initialize", "(Landroid/content/Context;)Z", (void*)Java_com_terista_space_native_NativeBridge_initialize},
    {"cleanup", "()V", (void*)Java_com_terista_space_native_NativeBridge_cleanup},
    {"installHook", "(Ljava/lang/String;JJ)Z", (void*)Java_com_terista_space_native_NativeBridge_installHook},
    {"uninstallHook", "(Ljava/lang/String;)Z", (void*)Java_com_terista_space_native_NativeBridge_uninstallHook},
    {"findSymbol", "(Ljava/lang/String;Ljava/lang/String;)J", (void*)Java_com_terista_space_native_NativeBridge_findSymbol},
    {"loadLibrary", "(Ljava/lang/String;)Z", (void*)Java_com_terista_space_native_NativeBridge_loadLibrary},
    {"hookBinder", "()Z", (void*)Java_com_terista_space_native_NativeBridge_hookBinder},
    {"unhookBinder", "()V", (void*)Java_com_terista_space_native_NativeBridge_unhookBinder},
    {"setBinderFilter", "(Ljava/lang/String;Z)Z", (void*)Java_com_terista_space_native_NativeBridge_setBinderFilter},
    {"createVirtualProcess", "(Ljava/lang/String;I)Z", (void*)Java_com_terista_space_native_NativeBridge_createVirtualProcess},
    {"killVirtualProcess", "(I)Z", (void*)Java_com_terista_space_native_NativeBridge_killVirtualProcess},
    {"protectMemory", "(JJI)Z", (void*)Java_com_terista_space_native_NativeBridge_protectMemory},
    {"allocateMemory", "(J)J", (void*)Java_com_terista_space_native_NativeBridge_allocateMemory},
    {"freeMemory", "(JJ)Z", (void*)Java_com_terista_space_native_NativeBridge_freeMemory}
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("JNI_OnLoad called");
    
    JNIEnv* env;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("Failed to get JNI environment");
        return -1;
    }
    
    // Find the class
    jclass clazz = env->FindClass("com/terista/space/native/NativeBridge");
    if (clazz == nullptr) {
        LOGE("Failed to find NativeBridge class");
        return -1;
    }
    
    // Register native methods
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        LOGE("Failed to register native methods");
        return -1;
    }
    
    LOGI("Native methods registered successfully");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    LOGI("JNI_OnUnload called");
    
    // Cleanup TeristaNative instance
    if (TeristaNative::getInstance()) {
        TeristaNative::getInstance()->cleanup();
    }
}

}