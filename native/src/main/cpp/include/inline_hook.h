#ifndef INLINE_HOOK_H
#define INLINE_HOOK_H

#include <vector>
#include <memory>
#include <map>

struct HookEntry {
    void* originalFunction;
    void* replacementFunction;
    void* backupFunction;
    unsigned char originalBytes[32];
    size_t originalSize;
    bool active;
};

/**
 * Inline hooking engine for function interception
 */
class InlineHook {
public:
    InlineHook();
    ~InlineHook();
    
    // Hook management
    bool installHook(void* originalFunction, void* replacementFunction, void** backupFunction);
    bool uninstallHook(void* originalFunction, void* backupFunction);
    bool isHookInstalled(void* originalFunction);
    
    // Architecture specific
    bool installArm64Hook(void* originalFunction, void* replacementFunction, void** backupFunction);
    bool installArm32Hook(void* originalFunction, void* replacementFunction, void** backupFunction);
    
    // Utility functions
    bool makeMemoryWritable(void* addr, size_t size);
    bool makeMemoryExecutable(void* addr, size_t size);
    void* allocateTrampoline();
    void freeTrampoline(void* trampoline);
    
private:
    std::map<void*, std::shared_ptr<HookEntry>> hooks;
    std::vector<void*> trampolines;
    
    // ARM64 specific functions
    bool writeArm64Jump(void* addr, void* target);
    size_t getArm64InstructionSize(void* addr);
    
    // ARM32 specific functions  
    bool writeArm32Jump(void* addr, void* target);
    size_t getArm32InstructionSize(void* addr);
    
    // Common utilities
    bool isValidAddress(void* addr);
    void flushInstructionCache(void* addr, size_t size);
};

#endif // INLINE_HOOK_H