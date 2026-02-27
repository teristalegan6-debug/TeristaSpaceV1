#include "include/inline_hook.h"
#include <android/log.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>

#ifdef __aarch64__
#include <asm/cachectl.h>
#endif

#define LOG_TAG "InlineHook"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

InlineHook::InlineHook() {
    LOGD("InlineHook initialized");
}

InlineHook::~InlineHook() {
    // Cleanup all hooks
    for (auto& pair : hooks) {
        if (pair.second->active) {
            uninstallHook(pair.first, pair.second->backupFunction);
        }
    }
    hooks.clear();
    
    // Free trampolines
    for (void* trampoline : trampolines) {
        munmap(trampoline, getpagesize());
    }
    trampolines.clear();
    
    LOGD("InlineHook destroyed");
}

bool InlineHook::installHook(void* originalFunction, void* replacementFunction, void** backupFunction) {
    if (!originalFunction || !replacementFunction) {
        LOGE("Invalid function pointers");
        return false;
    }
    
    LOGD("Installing hook: original=%p, replacement=%p", originalFunction, replacementFunction);
    
    // Check if already hooked
    if (isHookInstalled(originalFunction)) {
        LOGE("Function already hooked: %p", originalFunction);
        return false;
    }
    
    // Validate address
    if (!isValidAddress(originalFunction)) {
        LOGE("Invalid original function address: %p", originalFunction);
        return false;
    }
    
#ifdef __aarch64__
    return installArm64Hook(originalFunction, replacementFunction, backupFunction);
#else
    return installArm32Hook(originalFunction, replacementFunction, backupFunction);
#endif
}

bool InlineHook::uninstallHook(void* originalFunction, void* backupFunction) {
    auto it = hooks.find(originalFunction);
    if (it == hooks.end()) {
        LOGE("Hook not found for function: %p", originalFunction);
        return false;
    }
    
    auto& hook = it->second;
    if (!hook->active) {
        LOGE("Hook already inactive for function: %p", originalFunction);
        return false;
    }
    
    LOGD("Uninstalling hook: original=%p", originalFunction);
    
    // Make memory writable
    if (!makeMemoryWritable(originalFunction, hook->originalSize)) {
        LOGE("Failed to make memory writable for uninstalling hook");
        return false;
    }
    
    // Restore original bytes
    memcpy(originalFunction, hook->originalBytes, hook->originalSize);
    
    // Flush instruction cache
    flushInstructionCache(originalFunction, hook->originalSize);
    
    // Make memory executable again
    makeMemoryExecutable(originalFunction, hook->originalSize);
    
    hook->active = false;
    hooks.erase(it);
    
    LOGI("Hook uninstalled successfully: %p", originalFunction);
    return true;
}

bool InlineHook::isHookInstalled(void* originalFunction) {
    auto it = hooks.find(originalFunction);
    return it != hooks.end() && it->second->active;
}

#ifdef __aarch64__
bool InlineHook::installArm64Hook(void* originalFunction, void* replacementFunction, void** backupFunction) {
    LOGD("Installing ARM64 hook");
    
    // Create hook entry
    auto hook = std::make_shared<HookEntry>();
    hook->originalFunction = originalFunction;
    hook->replacementFunction = replacementFunction;
    hook->active = false;
    
    // Calculate instruction size needed (ARM64 uses 4-byte instructions)
    size_t hookSize = 16; // Need at least 4 instructions for long jump
    hook->originalSize = hookSize;
    
    // Make memory writable
    if (!makeMemoryWritable(originalFunction, hookSize)) {
        LOGE("Failed to make memory writable");
        return false;
    }
    
    // Backup original bytes
    memcpy(hook->originalBytes, originalFunction, hookSize);
    
    // Allocate trampoline
    void* trampoline = allocateTrampoline();
    if (!trampoline) {
        LOGE("Failed to allocate trampoline");
        return false;
    }
    
    // Build trampoline: original instructions + jump back
    memcpy(trampoline, originalFunction, hookSize);
    
    // Add jump back to original function + hookSize
    uint8_t* trampolineCode = (uint8_t*)trampoline;
    uintptr_t returnAddr = (uintptr_t)originalFunction + hookSize;
    uintptr_t trampolineAddr = (uintptr_t)trampoline + hookSize;
    
    // ARM64 absolute jump sequence
    // LDR X16, #8
    // BR X16
    // .quad target_address
    uint32_t ldr_instruction = 0x58000050; // LDR X16, #8
    uint32_t br_instruction = 0xD61F0200;  // BR X16
    
    *(uint32_t*)(trampolineAddr) = ldr_instruction;
    *(uint32_t*)(trampolineAddr + 4) = br_instruction;
    *(uint64_t*)(trampolineAddr + 8) = returnAddr;
    
    // Flush trampoline cache
    flushInstructionCache(trampoline, hookSize + 16);
    
    // Write jump to replacement in original function
    if (!writeArm64Jump(originalFunction, replacementFunction)) {
        LOGE("Failed to write ARM64 jump");
        freeTrampoline(trampoline);
        return false;
    }
    
    // Make memory executable
    makeMemoryExecutable(originalFunction, hookSize);
    
    hook->backupFunction = trampoline;
    hook->active = true;
    hooks[originalFunction] = hook;
    
    if (backupFunction) {
        *backupFunction = trampoline;
    }
    
    LOGI("ARM64 hook installed successfully");
    return true;
}

bool InlineHook::writeArm64Jump(void* addr, void* target) {
    uint8_t* code = (uint8_t*)addr;
    uintptr_t targetAddr = (uintptr_t)target;
    
    // ARM64 absolute jump sequence:
    // LDR X16, #8    (0x58000050)
    // BR X16         (0xD61F0200) 
    // .quad target   (target address)
    
    uint32_t ldr_instruction = 0x58000050; // LDR X16, #8
    uint32_t br_instruction = 0xD61F0200;  // BR X16
    
    *(uint32_t*)code = ldr_instruction;
    *(uint32_t*)(code + 4) = br_instruction;
    *(uint64_t*)(code + 8) = targetAddr;
    
    flushInstructionCache(addr, 16);
    return true;
}

size_t InlineHook::getArm64InstructionSize(void* addr) {
    // ARM64 instructions are always 4 bytes
    return 4;
}

#else // ARM32

bool InlineHook::installArm32Hook(void* originalFunction, void* replacementFunction, void** backupFunction) {
    LOGD("Installing ARM32 hook");
    
    // Create hook entry
    auto hook = std::make_shared<HookEntry>();
    hook->originalFunction = originalFunction;
    hook->replacementFunction = replacementFunction;
    hook->active = false;
    
    // Calculate instruction size needed (ARM32 uses 4-byte instructions in ARM mode)
    size_t hookSize = 8; // Need 2 instructions for jump
    hook->originalSize = hookSize;
    
    // Make memory writable
    if (!makeMemoryWritable(originalFunction, hookSize)) {
        LOGE("Failed to make memory writable");
        return false;
    }
    
    // Backup original bytes
    memcpy(hook->originalBytes, originalFunction, hookSize);
    
    // Allocate trampoline
    void* trampoline = allocateTrampoline();
    if (!trampoline) {
        LOGE("Failed to allocate trampoline");
        return false;
    }
    
    // Build trampoline
    memcpy(trampoline, originalFunction, hookSize);
    
    // Add jump back
    uint8_t* trampolineCode = (uint8_t*)trampoline;
    uintptr_t returnAddr = (uintptr_t)originalFunction + hookSize;
    
    // ARM32 absolute jump
    *(uint32_t*)(trampolineCode + hookSize) = 0xe51ff004; // LDR PC, [PC, #-4]
    *(uint32_t*)(trampolineCode + hookSize + 4) = returnAddr;
    
    flushInstructionCache(trampoline, hookSize + 8);
    
    // Write jump to replacement
    if (!writeArm32Jump(originalFunction, replacementFunction)) {
        LOGE("Failed to write ARM32 jump");
        freeTrampoline(trampoline);
        return false;
    }
    
    makeMemoryExecutable(originalFunction, hookSize);
    
    hook->backupFunction = trampoline;
    hook->active = true;
    hooks[originalFunction] = hook;
    
    if (backupFunction) {
        *backupFunction = trampoline;
    }
    
    LOGI("ARM32 hook installed successfully");
    return true;
}

bool InlineHook::writeArm32Jump(void* addr, void* target) {
    uint8_t* code = (uint8_t*)addr;
    uintptr_t targetAddr = (uintptr_t)target;
    
    // ARM32 absolute jump:
    // LDR PC, [PC, #-4]  (0xe51ff004)
    // .word target
    
    *(uint32_t*)code = 0xe51ff004; // LDR PC, [PC, #-4]
    *(uint32_t*)(code + 4) = targetAddr;
    
    flushInstructionCache(addr, 8);
    return true;
}

size_t InlineHook::getArm32InstructionSize(void* addr) {
    // ARM32 instructions are 4 bytes in ARM mode, 2 bytes in Thumb mode
    // For simplicity, assume ARM mode
    return 4;
}

#endif

bool InlineHook::makeMemoryWritable(void* addr, size_t size) {
    uintptr_t pageAddr = (uintptr_t)addr & ~(getpagesize() - 1);
    size_t pageSize = ((uintptr_t)addr + size - pageAddr + getpagesize() - 1) & ~(getpagesize() - 1);
    
    if (mprotect((void*)pageAddr, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        LOGE("Failed to make memory writable: %p, size: %zu", addr, size);
        return false;
    }
    
    return true;
}

bool InlineHook::makeMemoryExecutable(void* addr, size_t size) {
    uintptr_t pageAddr = (uintptr_t)addr & ~(getpagesize() - 1);
    size_t pageSize = ((uintptr_t)addr + size - pageAddr + getpagesize() - 1) & ~(getpagesize() - 1);
    
    if (mprotect((void*)pageAddr, pageSize, PROT_READ | PROT_EXEC) != 0) {
        LOGE("Failed to make memory executable: %p, size: %zu", addr, size);
        return false;
    }
    
    return true;
}

void* InlineHook::allocateTrampoline() {
    size_t trampolineSize = getpagesize();
    
    void* trampoline = mmap(nullptr, trampolineSize, 
                           PROT_READ | PROT_WRITE | PROT_EXEC,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (trampoline == MAP_FAILED) {
        LOGE("Failed to allocate trampoline");
        return nullptr;
    }
    
    trampolines.push_back(trampoline);
    LOGD("Trampoline allocated: %p", trampoline);
    return trampoline;
}

void InlineHook::freeTrampoline(void* trampoline) {
    if (!trampoline) return;
    
    auto it = std::find(trampolines.begin(), trampolines.end(), trampoline);
    if (it != trampolines.end()) {
        munmap(trampoline, getpagesize());
        trampolines.erase(it);
        LOGD("Trampoline freed: %p", trampoline);
    }
}

bool InlineHook::isValidAddress(void* addr) {
    // Basic validation - check if address is in valid range
    uintptr_t address = (uintptr_t)addr;
    return address > 0x1000 && address < 0x7fffffff00000000ULL;
}

void InlineHook::flushInstructionCache(void* addr, size_t size) {
#ifdef __aarch64__
    // Use cacheflush syscall or builtin
    __builtin___clear_cache((char*)addr, (char*)addr + size);
#else
    // ARM32 cache flush
    __builtin___clear_cache((char*)addr, (char*)addr + size);
#endif
}