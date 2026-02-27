#ifndef TERISTA_NATIVE_H
#define TERISTA_NATIVE_H

#include <jni.h>
#include <string>
#include <vector>
#include <memory>

// Forward declarations
struct HookInfo;
class ElfUtils;
class InlineHook;
class BinderHook;
class SymbolResolver;

/**
 * Main native interface for TeristaSpace virtualization engine
 */
class TeristaNative {
public:
    static TeristaNative* getInstance();
    
    // Initialization
    bool initialize(JNIEnv* env, jobject context);
    void cleanup();
    
    // Hook management
    bool installHook(const std::string& symbol, void* replacement, void** backup);
    bool uninstallHook(const std::string& symbol);
    void uninstallAllHooks();
    
    // ELF manipulation
    bool loadLibrary(const std::string& path);
    void* findSymbol(const std::string& libname, const std::string& symbol);
    
    // Binder interception
    bool hookBinder();
    void unhookBinder();
    bool setBinderFilter(const std::string& serviceName, bool allow);
    
    // Process management  
    bool createVirtualProcess(const std::string& packageName, int userId);
    bool killVirtualProcess(int pid);
    
    // Memory management
    bool protectMemory(void* addr, size_t size, int prot);
    void* allocateMemory(size_t size);
    bool freeMemory(void* addr, size_t size);
    
private:
    TeristaNative();
    ~TeristaNative();
    
    static TeristaNative* instance;
    
    std::unique_ptr<ElfUtils> elfUtils;
    std::unique_ptr<InlineHook> inlineHook;
    std::unique_ptr<BinderHook> binderHook;
    std::unique_ptr<SymbolResolver> symbolResolver;
    
    bool initialized;
    JNIEnv* jniEnv;
    jobject applicationContext;
    
    std::vector<std::shared_ptr<HookInfo>> activeHooks;
};

// Hook information structure
struct HookInfo {
    std::string symbol;
    void* originalFunction;
    void* replacementFunction;
    void* backupFunction;
    bool active;
    
    HookInfo(const std::string& sym, void* orig, void* repl, void* backup) :
        symbol(sym), originalFunction(orig), replacementFunction(repl), 
        backupFunction(backup), active(true) {}
};

// Utility macros
#define TERISTA_LOG_TAG "TeristaNative"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TERISTA_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TERISTA_LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TERISTA_LOG_TAG, __VA_ARGS__)

#endif // TERISTA_NATIVE_H