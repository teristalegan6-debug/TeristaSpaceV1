#include "include/symbol_resolver.h"
#include <android/log.h>
#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include <regex>
#include <fstream>

#define LOG_TAG "SymbolResolver"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

SymbolResolver::SymbolResolver() {
    LOGD("SymbolResolver initialized");
}

SymbolResolver::~SymbolResolver() {
    clearCache();
    LOGD("SymbolResolver destroyed");
}

void* SymbolResolver::resolveSymbol(const std::string& symbolName) {
    LOGD("Resolving symbol: %s", symbolName.c_str());
    
    // Check cache first
    auto it = symbolCache.find(symbolName);
    if (it != symbolCache.end()) {
        LOGD("Symbol found in cache: %s at %p", symbolName.c_str(), it->second.address);
        return it->second.address;
    }
    
    // Try dlsym with RTLD_DEFAULT
    void* addr = dlsym(RTLD_DEFAULT, symbolName.c_str());
    if (addr) {
        // Create symbol info and cache it
        SymbolInfo info;
        info.name = symbolName;
        info.address = addr;
        info.size = 0; // Size unknown from dlsym
        info.library = getLibraryName(addr);
        info.isFunction = true; // Assume function for now
        info.isExported = true;
        
        symbolCache[symbolName] = info;
        LOGD("Symbol resolved via RTLD_DEFAULT: %s at %p", symbolName.c_str(), addr);
        return addr;
    }
    
    // Scan all libraries if not found
    if (!scannedLibraries.empty()) {
        for (const std::string& libPath : scannedLibraries) {
            addr = dlsymWrapper(libPath, symbolName);
            if (addr) {
                SymbolInfo info;
                info.name = symbolName;
                info.address = addr;
                info.size = 0;
                info.library = libPath;
                info.isFunction = true;
                info.isExported = true;
                
                symbolCache[symbolName] = info;
                LOGD("Symbol resolved from scanned library: %s at %p", symbolName.c_str(), addr);
                return addr;
            }
        }
    }
    
    LOGE("Symbol not found: %s", symbolName.c_str());
    return nullptr;
}

void* SymbolResolver::resolveSymbol(const std::string& libraryName, const std::string& symbolName) {
    LOGD("Resolving symbol %s in library %s", symbolName.c_str(), libraryName.c_str());
    
    // Create cache key
    std::string cacheKey = libraryName + "::" + symbolName;
    auto it = symbolCache.find(cacheKey);
    if (it != symbolCache.end()) {
        LOGD("Symbol found in cache: %s", cacheKey.c_str());
        return it->second.address;
    }
    
    void* addr = dlsymWrapper(libraryName, symbolName);
    if (addr) {
        SymbolInfo info;
        info.name = symbolName;
        info.address = addr;
        info.size = 0;
        info.library = libraryName;
        info.isFunction = true;
        info.isExported = true;
        
        symbolCache[cacheKey] = info;
        LOGD("Symbol resolved: %s in %s at %p", symbolName.c_str(), libraryName.c_str(), addr);
        return addr;
    }
    
    LOGE("Symbol not found: %s in %s", symbolName.c_str(), libraryName.c_str());
    return nullptr;
}

std::vector<SymbolInfo> SymbolResolver::findSymbols(const std::string& pattern) {
    std::vector<SymbolInfo> results;
    
    try {
        std::regex regex(pattern);
        
        for (const auto& pair : symbolCache) {
            if (std::regex_search(pair.second.name, regex)) {
                results.push_back(pair.second);
            }
        }
    } catch (const std::regex_error& e) {
        LOGE("Invalid regex pattern: %s", pattern.c_str());
    }
    
    LOGD("Found %zu symbols matching pattern: %s", results.size(), pattern.c_str());
    return results;
}

bool SymbolResolver::scanLibrary(const std::string& libraryPath) {
    LOGD("Scanning library: %s", libraryPath.c_str());
    
    // Check if already scanned
    auto it = std::find(scannedLibraries.begin(), scannedLibraries.end(), libraryPath);
    if (it != scannedLibraries.end()) {
        LOGD("Library already scanned: %s", libraryPath.c_str());
        return true;
    }
    
    // Try to parse ELF symbols
    if (parseElfSymbols(libraryPath)) {
        scannedLibraries.push_back(libraryPath);
        LOGI("Library scanned successfully: %s", libraryPath.c_str());
        return true;
    }
    
    LOGE("Failed to scan library: %s", libraryPath.c_str());
    return false;
}

bool SymbolResolver::scanAllLibraries() {
    LOGI("Scanning all loaded libraries...");
    
    // Read /proc/self/maps to get loaded libraries
    std::ifstream maps("/proc/self/maps");
    if (!maps.is_open()) {
        LOGE("Failed to open /proc/self/maps");
        return false;
    }
    
    std::string line;
    std::set<std::string> libraries;
    
    while (std::getline(maps, line)) {
        // Parse maps line: address perms offset dev inode pathname
        size_t pathStart = line.find_last_of(' ');
        if (pathStart != std::string::npos) {
            std::string path = line.substr(pathStart + 1);
            
            // Filter for shared libraries
            if (path.find(".so") != std::string::npos && path[0] == '/') {
                libraries.insert(path);
            }
        }
    }
    
    // Scan each library
    size_t scannedCount = 0;
    for (const std::string& libPath : libraries) {
        if (scanLibrary(libPath)) {
            scannedCount++;
        }
    }
    
    LOGI("Scanned %zu libraries out of %zu found", scannedCount, libraries.size());
    return scannedCount > 0;
}

void SymbolResolver::clearCache() {
    symbolCache.clear();
    librarySymbols.clear();
    scannedLibraries.clear();
    LOGD("Symbol cache cleared");
}

SymbolInfo SymbolResolver::getSymbolInfo(const std::string& symbolName) {
    auto it = symbolCache.find(symbolName);
    if (it != symbolCache.end()) {
        return it->second;
    }
    
    // Return empty symbol info if not found
    SymbolInfo empty;
    empty.name = symbolName;
    empty.address = nullptr;
    empty.size = 0;
    empty.isFunction = false;
    empty.isExported = false;
    return empty;
}

std::vector<std::string> SymbolResolver::getLibrarySymbols(const std::string& libraryName) {
    auto it = librarySymbols.find(libraryName);
    if (it != librarySymbols.end()) {
        return it->second;
    }
    return std::vector<std::string>();
}

std::vector<std::string> SymbolResolver::getLoadedLibraries() {
    return scannedLibraries;
}

std::string SymbolResolver::getSymbolName(void* address) {
    // Use dladdr to get symbol info
    Dl_info info;
    if (dladdr(address, &info) && info.dli_sname) {
        return std::string(info.dli_sname);
    }
    
    // Search in cache
    for (const auto& pair : symbolCache) {
        if (pair.second.address == address) {
            return pair.second.name;
        }
    }
    
    return "";
}

std::string SymbolResolver::getLibraryName(void* address) {
    Dl_info info;
    if (dladdr(address, &info) && info.dli_fname) {
        return std::string(info.dli_fname);
    }
    return "";
}

bool SymbolResolver::isValidSymbolAddress(void* address) {
    if (!address) return false;
    
    // Check if address is in valid memory range
    uintptr_t addr = (uintptr_t)address;
    return addr > 0x1000; // Basic check
}

bool SymbolResolver::parseElfSymbols(const std::string& libraryPath) {
    LOGD("Parsing ELF symbols for: %s", libraryPath.c_str());
    
    int fd = open(libraryPath.c_str(), O_RDONLY);
    if (fd < 0) {
        LOGE("Failed to open library: %s", libraryPath.c_str());
        return false;
    }
    
    // Read ELF header
    Elf64_Ehdr ehdr;
    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
        LOGE("Failed to read ELF header");
        close(fd);
        return false;
    }
    
    // Verify ELF magic
    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        LOGE("Invalid ELF magic");
        close(fd);
        return false;
    }
    
    // For now, use a basic approach with dlopen
    close(fd);
    
    void* handle = dlopen(libraryPath.c_str(), RTLD_LAZY);
    if (!handle) {
        LOGE("Failed to dlopen library: %s", libraryPath.c_str());
        return false;
    }
    
    // Store library reference
    std::vector<std::string> symbols;
    librarySymbols[libraryPath] = symbols;
    
    dlclose(handle);
    return true;
}

bool SymbolResolver::parseDynamicSymbols(const std::string& libraryPath) {
    // Implementation for parsing dynamic symbol table
    // This would be more complex ELF parsing
    return true;
}

void* SymbolResolver::dlsymWrapper(const std::string& libraryName, const std::string& symbolName) {
    void* handle = dlopen(libraryName.c_str(), RTLD_LAZY);
    if (!handle) {
        return nullptr;
    }
    
    void* symbol = dlsym(handle, symbolName.c_str());
    
    // Don't close handle to keep symbol valid
    // dlclose(handle);
    
    return symbol;
}

bool SymbolResolver::isSymbolVisible(const std::string& symbolName) {
    // Check if symbol is exported and visible
    return symbolName.find("__") != 0; // Skip internal symbols starting with __
}

std::string SymbolResolver::demangle(const std::string& mangledName) {
    // Basic demangling - would need more sophisticated implementation
    // For now, just return the original name
    return mangledName;
}

void SymbolResolver::addSymbol(const SymbolInfo& symbol) {
    symbolCache[symbol.name] = symbol;
    
    // Add to library symbols list
    if (!symbol.library.empty()) {
        librarySymbols[symbol.library].push_back(symbol.name);
    }
}