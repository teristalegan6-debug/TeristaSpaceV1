#ifndef BINDER_HOOK_H
#define BINDER_HOOK_H

#include <string>
#include <vector>
#include <map>
#include <functional>

// Binder transaction codes
#define BINDER_TYPE_BINDER 0x73622a85
#define BINDER_TYPE_WEAK_BINDER 0x77622a85
#define BINDER_TYPE_HANDLE 0x68622a85
#define BINDER_TYPE_WEAK_HANDLE 0x77682a85

struct BinderTransaction {
    uint32_t code;
    uint32_t flags;
    std::string target;
    std::vector<uint8_t> data;
    std::vector<uint8_t> reply;
};

typedef std::function<bool(BinderTransaction&)> BinderFilter;

/**
 * Binder IPC interception and filtering system
 */
class BinderHook {
public:
    BinderHook();
    ~BinderHook();
    
    // Hook management
    bool installHooks();
    void uninstallHooks();
    bool isHooked();
    
    // Service filtering
    bool setServiceFilter(const std::string& serviceName, bool allow);
    bool removeServiceFilter(const std::string& serviceName);
    void clearServiceFilters();
    
    // Transaction filtering
    bool addTransactionFilter(const std::string& serviceName, BinderFilter filter);
    bool removeTransactionFilter(const std::string& serviceName);
    
    // Hook callbacks
    static int hooked_ioctl(int fd, unsigned long request, void* arg);
    static ssize_t hooked_write(int fd, const void* buf, size_t count);
    static ssize_t hooked_read(int fd, void* buf, size_t count);
    
    // Transaction processing
    bool processTransaction(BinderTransaction& transaction);
    bool allowService(const std::string& serviceName);
    
private:
    bool hooksInstalled;
    std::map<std::string, bool> serviceFilters;
    std::map<std::string, BinderFilter> transactionFilters;
    
    // Original function pointers
    static int (*original_ioctl)(int fd, unsigned long request, void* arg);
    static ssize_t (*original_write)(int fd, const void* buf, size_t count);
    static ssize_t (*original_read)(int fd, void* buf, size_t count);
    
    // Binder device handling
    bool isBinderDevice(int fd);
    std::string getServiceName(const void* data, size_t size);
    bool parseBinderTransaction(const void* data, size_t size, BinderTransaction& transaction);
    
    // Default filters
    void setupDefaultFilters();
};

#endif // BINDER_HOOK_H