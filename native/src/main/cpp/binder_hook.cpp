#include "include/binder_hook.h"
#include "include/terista_native.h"
#include <android/log.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

#define LOG_TAG "BinderHook"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Binder ioctl commands
#define BINDER_WRITE_READ _IOWR('b', 1, struct binder_write_read)
#define BINDER_SET_IDLE_TIMEOUT _IOW('b', 3, __u64)
#define BINDER_SET_MAX_THREADS _IOW('b', 5, __u32)
#define BINDER_SET_IDLE_PRIORITY _IOW('b', 6, __s32)
#define BINDER_SET_CONTEXT_MGR _IOW('b', 7, __s32)
#define BINDER_THREAD_EXIT _IOW('b', 8, __s32)
#define BINDER_VERSION _IOWR('b', 9, struct binder_version)

// Static member definitions
int (*BinderHook::original_ioctl)(int fd, unsigned long request, void* arg) = nullptr;
ssize_t (*BinderHook::original_write)(int fd, const void* buf, size_t count) = nullptr;
ssize_t (*BinderHook::original_read)(int fd, void* buf, size_t count) = nullptr;

BinderHook::BinderHook() : hooksInstalled(false) {
    setupDefaultFilters();
    LOGD("BinderHook initialized");
}

BinderHook::~BinderHook() {
    uninstallHooks();
    LOGD("BinderHook destroyed");
}

bool BinderHook::installHooks() {
    if (hooksInstalled) {
        LOGD("Binder hooks already installed");
        return true;
    }
    
    LOGI("Installing Binder hooks...");
    
    TeristaNative* native = TeristaNative::getInstance();
    
    // Hook ioctl
    void* ioctlBackup = nullptr;
    if (!native->installHook("ioctl", (void*)hooked_ioctl, &ioctlBackup)) {
        LOGE("Failed to hook ioctl");
        return false;
    }
    original_ioctl = (int(*)(int, unsigned long, void*))ioctlBackup;
    
    // Hook write
    void* writeBackup = nullptr;
    if (!native->installHook("write", (void*)hooked_write, &writeBackup)) {
        LOGE("Failed to hook write");
        native->uninstallHook("ioctl");
        return false;
    }
    original_write = (ssize_t(*)(int, const void*, size_t))writeBackup;
    
    // Hook read
    void* readBackup = nullptr;
    if (!native->installHook("read", (void*)hooked_read, &readBackup)) {
        LOGE("Failed to hook read");
        native->uninstallHook("ioctl");
        native->uninstallHook("write");
        return false;
    }
    original_read = (ssize_t(*)(int, void*, size_t))readBackup;
    
    hooksInstalled = true;
    LOGI("Binder hooks installed successfully");
    return true;
}

void BinderHook::uninstallHooks() {
    if (!hooksInstalled) return;
    
    LOGI("Uninstalling Binder hooks...");
    
    TeristaNative* native = TeristaNative::getInstance();
    native->uninstallHook("ioctl");
    native->uninstallHook("write");
    native->uninstallHook("read");
    
    original_ioctl = nullptr;
    original_write = nullptr;
    original_read = nullptr;
    
    hooksInstalled = false;
    LOGI("Binder hooks uninstalled");
}

bool BinderHook::isHooked() {
    return hooksInstalled;
}

bool BinderHook::setServiceFilter(const std::string& serviceName, bool allow) {
    LOGD("Setting service filter: %s -> %s", serviceName.c_str(), allow ? "ALLOW" : "BLOCK");
    serviceFilters[serviceName] = allow;
    return true;
}

bool BinderHook::removeServiceFilter(const std::string& serviceName) {
    auto it = serviceFilters.find(serviceName);
    if (it != serviceFilters.end()) {
        serviceFilters.erase(it);
        LOGD("Service filter removed: %s", serviceName.c_str());
        return true;
    }
    return false;
}

void BinderHook::clearServiceFilters() {
    serviceFilters.clear();
    setupDefaultFilters();
    LOGD("Service filters cleared and defaults restored");
}

bool BinderHook::addTransactionFilter(const std::string& serviceName, BinderFilter filter) {
    transactionFilters[serviceName] = filter;
    LOGD("Transaction filter added for service: %s", serviceName.c_str());
    return true;
}

bool BinderHook::removeTransactionFilter(const std::string& serviceName) {
    auto it = transactionFilters.find(serviceName);
    if (it != transactionFilters.end()) {
        transactionFilters.erase(it);
        LOGD("Transaction filter removed for service: %s", serviceName.c_str());
        return true;
    }
    return false;
}

int BinderHook::hooked_ioctl(int fd, unsigned long request, void* arg) {
    // Check if this is a binder device
    if (request == BINDER_WRITE_READ) {
        LOGD("Intercepted binder ioctl: fd=%d, request=0x%lx", fd, request);
        
        // Process binder transaction if needed
        // For now, just log and pass through
        
        // TODO: Parse binder_write_read structure and filter transactions
    }
    
    // Call original ioctl
    if (original_ioctl) {
        return original_ioctl(fd, request, arg);
    }
    
    return -1;
}

ssize_t BinderHook::hooked_write(int fd, const void* buf, size_t count) {
    // Check if this is a binder device write
    if (buf && count > 0) {
        // Basic binder device detection (could be improved)
        if (count >= 4) {
            uint32_t cmd = *(uint32_t*)buf;
            if (cmd == BINDER_TYPE_BINDER || cmd == BINDER_TYPE_HANDLE) {
                LOGD("Intercepted potential binder write: fd=%d, count=%zu", fd, count);
            }
        }
    }
    
    // Call original write
    if (original_write) {
        return original_write(fd, buf, count);
    }
    
    return -1;
}

ssize_t BinderHook::hooked_read(int fd, void* buf, size_t count) {
    // Call original read first
    ssize_t result = -1;
    if (original_read) {
        result = original_read(fd, buf, count);
    }
    
    // Process result if it's a binder read
    if (result > 0 && buf) {
        // Basic binder detection
        if (result >= 4) {
            uint32_t cmd = *(uint32_t*)buf;
            if (cmd == BINDER_TYPE_BINDER || cmd == BINDER_TYPE_HANDLE) {
                LOGD("Intercepted potential binder read: fd=%d, count=%zu, result=%zd", fd, count, result);
            }
        }
    }
    
    return result;
}

bool BinderHook::processTransaction(BinderTransaction& transaction) {
    LOGD("Processing binder transaction: target=%s, code=%u", 
         transaction.target.c_str(), transaction.code);
    
    // Check service filter
    if (!allowService(transaction.target)) {
        LOGD("Service blocked by filter: %s", transaction.target.c_str());
        return false;
    }
    
    // Check transaction filter
    auto filterIt = transactionFilters.find(transaction.target);
    if (filterIt != transactionFilters.end()) {
        if (!filterIt->second(transaction)) {
            LOGD("Transaction blocked by custom filter: %s", transaction.target.c_str());
            return false;
        }
    }
    
    return true;
}

bool BinderHook::allowService(const std::string& serviceName) {
    auto it = serviceFilters.find(serviceName);
    if (it != serviceFilters.end()) {
        return it->second;
    }
    
    // Default to allow if no specific filter
    return true;
}

bool BinderHook::isBinderDevice(int fd) {
    // Check if fd points to /dev/binder or similar
    char fdPath[64];
    char linkTarget[256];
    
    snprintf(fdPath, sizeof(fdPath), "/proc/self/fd/%d", fd);
    
    ssize_t len = readlink(fdPath, linkTarget, sizeof(linkTarget) - 1);
    if (len > 0) {
        linkTarget[len] = '\0';
        return strstr(linkTarget, "binder") != nullptr;
    }
    
    return false;
}

std::string BinderHook::getServiceName(const void* data, size_t size) {
    // Basic service name extraction from binder data
    // This is a simplified implementation
    
    if (!data || size < 16) {
        return "";
    }
    
    // Look for string patterns that might be service names
    const char* str = (const char*)data;
    for (size_t i = 0; i < size - 4; i++) {
        if (str[i] >= 'a' && str[i] <= 'z' && str[i+1] >= 'a' && str[i+1] <= 'z') {
            // Found potential service name start
            size_t len = 0;
            while (i + len < size && 
                   ((str[i + len] >= 'a' && str[i + len] <= 'z') ||
                    (str[i + len] >= 'A' && str[i + len] <= 'Z') ||
                    (str[i + len] >= '0' && str[i + len] <= '9') ||
                    str[i + len] == '.' || str[i + len] == '_')) {
                len++;
            }
            
            if (len > 3) {
                return std::string(str + i, len);
            }
        }
    }
    
    return "";
}

bool BinderHook::parseBinderTransaction(const void* data, size_t size, BinderTransaction& transaction) {
    // Basic binder transaction parsing
    // This is a simplified implementation
    
    if (!data || size < sizeof(uint32_t) * 2) {
        return false;
    }
    
    const uint32_t* words = (const uint32_t*)data;
    transaction.code = words[0];
    transaction.flags = words[1];
    
    // Extract target service name
    transaction.target = getServiceName(data, size);
    
    // Copy data
    transaction.data.assign((const uint8_t*)data, (const uint8_t*)data + size);
    
    return true;
}

void BinderHook::setupDefaultFilters() {
    // Allow essential system services by default
    serviceFilters["servicemanager"] = true;
    serviceFilters["package"] = true;
    serviceFilters["activity"] = true;
    serviceFilters["window"] = true;
    serviceFilters["input"] = true;
    serviceFilters["power"] = true;
    
    // Block potentially sensitive services by default
    serviceFilters["telephony.registry"] = false;
    serviceFilters["isms"] = false; // SMS service
    serviceFilters["phone"] = false;
    
    LOGD("Default service filters configured");
}