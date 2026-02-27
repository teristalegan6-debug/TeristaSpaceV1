#include "include/terista_native.h"
#include "include/elf_utils.h"
#include "include/inline_hook.h"
#include "include/binder_hook.h"
#include "include/symbol_resolver.h"

#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/mman.h>

TeristaNative* TeristaNative::instance = nullptr;

// JNI function declarations
extern "C" {
    JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_initialize(JNIEnv* env, jobject thiz, jobject context);
    JNIEXPORT void JNICALL Java_com_terista_space_native_NativeBridge_cleanup(JNIEnv* env, jobject thiz);
    JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_installHook(JNIEnv* env, jobject thiz, jstring symbol, jlong replacement, jlong backup);
    JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_uninstallHook(JNIEnv* env, jobject thiz, jstring symbol);
    JNIEXPORT jlong JNICALL Java_com_terista_space_native_NativeBridge_findSymbol(JNIEnv* env, jobject thiz, jstring libname, jstring symbol);
    JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_hookBinder(JNIEnv* env, jobject thiz);
}

TeristaNative::TeristaNative() :
    elfUtils(std::make_unique<ElfUtils>()),
    inlineHook(std::make_unique<InlineHook>()),
    binderHook(std::make_unique<BinderHook>()),
    symbolResolver(std::make_unique<SymbolResolver>()),
    initialized(false),
    jniEnv(nullptr),
    applicationContext(nullptr) {
}

TeristaNative::~TeristaNative() {
    cleanup();
}

TeristaNative* TeristaNative::getInstance() {
    if (instance == nullptr) {
        instance = new TeristaNative();
    }
    return instance;
}

bool TeristaNative::initialize(JNIEnv* env, jobject context) {
    if (initialized) {
        LOGD("TeristaNative already initialized");
        return true;
    }
    
    jniEnv = env;
    applicationContext = env->NewGlobalRef(context);
    
    LOGI("Initializing TeristaNative...");
    
    // Initialize subcomponents
    if (!elfUtils || !inlineHook || !binderHook || !symbolResolver) {
        LOGE("Failed to create native components");
        return false;
    }
    
    // Load system libraries for hooking
    if (!elfUtils->loadLibrary("libc.so") || 
        !elfUtils->loadLibrary("libdl.so") ||
        !elfUtils->loadLibrary("libbinder.so")) {
        LOGE("Failed to load system libraries");
        return false;
    }
    
    initialized = true;
    LOGI("TeristaNative initialized successfully");
    return true;
}

void TeristaNative::cleanup() {
    if (!initialized) return;
    
    LOGI("Cleaning up TeristaNative...");
    
    // Uninstall all hooks
    uninstallAllHooks();
    
    // Cleanup Binder hooks
    unhookBinder();
    
    // Release global reference
    if (applicationContext && jniEnv) {
        jniEnv->DeleteGlobalRef(applicationContext);
        applicationContext = nullptr;
    }
    
    // Reset components
    activeHooks.clear();
    
    initialized = false;
    jniEnv = nullptr;
    
    LOGI("TeristaNative cleanup completed");
}

bool TeristaNative::installHook(const std::string& symbol, void* replacement, void** backup) {
    if (!initialized || !inlineHook) {
        LOGE("TeristaNative not initialized");
        return false;
    }
    
    LOGD("Installing hook for symbol: %s", symbol.c_str());
    
    // Find the original function
    void* originalFunction = elfUtils->findSymbolInAll(symbol);
    if (!originalFunction) {
        LOGE("Symbol not found: %s", symbol.c_str());
        return false;
    }
    
    // Install inline hook
    void* backupFunction = nullptr;
    if (!inlineHook->installHook(originalFunction, replacement, &backupFunction)) {
        LOGE("Failed to install inline hook for: %s", symbol.c_str());
        return false;
    }
    
    // Store hook info
    auto hookInfo = std::make_shared<HookInfo>(symbol, originalFunction, replacement, backupFunction);
    activeHooks.push_back(hookInfo);
    
    if (backup) {
        *backup = backupFunction;
    }
    
    LOGI("Hook installed successfully for: %s", symbol.c_str());
    return true;
}

bool TeristaNative::uninstallHook(const std::string& symbol) {
    if (!initialized || !inlineHook) {
        LOGE("TeristaNative not initialized");
        return false;
    }
    
    LOGD("Uninstalling hook for symbol: %s", symbol.c_str());
    
    // Find the hook
    for (auto it = activeHooks.begin(); it != activeHooks.end(); ++it) {
        if ((*it)->symbol == symbol && (*it)->active) {
            if (inlineHook->uninstallHook((*it)->originalFunction, (*it)->backupFunction)) {
                (*it)->active = false;
                activeHooks.erase(it);
                LOGI("Hook uninstalled successfully for: %s", symbol.c_str());
                return true;
            }
            break;
        }
    }
    
    LOGE("Hook not found or failed to uninstall: %s", symbol.c_str());
    return false;
}

void TeristaNative::uninstallAllHooks() {
    LOGI("Uninstalling all hooks...");
    
    for (auto& hookInfo : activeHooks) {
        if (hookInfo->active) {
            inlineHook->uninstallHook(hookInfo->originalFunction, hookInfo->backupFunction);
            hookInfo->active = false;
        }
    }
    
    activeHooks.clear();
    LOGI("All hooks uninstalled");
}

bool TeristaNative::loadLibrary(const std::string& path) {
    if (!initialized || !elfUtils) return false;
    return elfUtils->loadLibrary(path);
}

void* TeristaNative::findSymbol(const std::string& libname, const std::string& symbol) {
    if (!initialized || !elfUtils) return nullptr;
    return elfUtils->findSymbol(libname, symbol);
}

bool TeristaNative::hookBinder() {
    if (!initialized || !binderHook) return false;
    return binderHook->installHooks();
}

void TeristaNative::unhookBinder() {
    if (!initialized || !binderHook) return;
    binderHook->uninstallHooks();
}

bool TeristaNative::setBinderFilter(const std::string& serviceName, bool allow) {
    if (!initialized || !binderHook) return false;
    return binderHook->setServiceFilter(serviceName, allow);
}

bool TeristaNative::protectMemory(void* addr, size_t size, int prot) {
    if (mprotect(addr, size, prot) == 0) {
        return true;
    }
    LOGE("Failed to protect memory at %p, size: %zu", addr, size);
    return false;
}

void* TeristaNative::allocateMemory(size_t size) {
    void* addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        LOGE("Failed to allocate memory, size: %zu", size);
        return nullptr;
    }
    return addr;
}

bool TeristaNative::freeMemory(void* addr, size_t size) {
    if (munmap(addr, size) == 0) {
        return true;
    }
    LOGE("Failed to free memory at %p, size: %zu", addr, size);
    return false;
}

// JNI implementations
extern "C" {

JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_initialize(JNIEnv* env, jobject thiz, jobject context) {
    return TeristaNative::getInstance()->initialize(env, context) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_com_terista_space_native_NativeBridge_cleanup(JNIEnv* env, jobject thiz) {
    TeristaNative::getInstance()->cleanup();
}

JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_installHook(JNIEnv* env, jobject thiz, jstring symbol, jlong replacement, jlong backup) {
    const char* symbolStr = env->GetStringUTFChars(symbol, nullptr);
    void* backupPtr = nullptr;
    bool result = TeristaNative::getInstance()->installHook(symbolStr, (void*)replacement, &backupPtr);
    
    // Store backup pointer if provided
    if (backup != 0) {
        *(void**)backup = backupPtr;
    }
    
    env->ReleaseStringUTFChars(symbol, symbolStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_uninstallHook(JNIEnv* env, jobject thiz, jstring symbol) {
    const char* symbolStr = env->GetStringUTFChars(symbol, nullptr);
    bool result = TeristaNative::getInstance()->uninstallHook(symbolStr);
    env->ReleaseStringUTFChars(symbol, symbolStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL Java_com_terista_space_native_NativeBridge_findSymbol(JNIEnv* env, jobject thiz, jstring libname, jstring symbol) {
    const char* libnameStr = env->GetStringUTFChars(libname, nullptr);
    const char* symbolStr = env->GetStringUTFChars(symbol, nullptr);
    
    void* result = TeristaNative::getInstance()->findSymbol(libnameStr, symbolStr);
    
    env->ReleaseStringUTFChars(libname, libnameStr);
    env->ReleaseStringUTFChars(symbol, symbolStr);
    
    return (jlong)result;
}

JNIEXPORT jboolean JNICALL Java_com_terista_space_native_NativeBridge_hookBinder(JNIEnv* env, jobject thiz) {
    return TeristaNative::getInstance()->hookBinder() ? JNI_TRUE : JNI_FALSE;
}

}