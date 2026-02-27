#include "include/elf_utils.h"
#include <android/log.h>
#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>

#define LOG_TAG "ElfUtils"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

ElfUtils::ElfUtils() {
    LOGD("ElfUtils initialized");
}

ElfUtils::~ElfUtils() {
    // Cleanup loaded libraries
    for (auto& pair : loadedLibraries) {
        if (pair.second && pair.second->baseAddress) {
            dlclose(pair.second->baseAddress);
        }
    }
    loadedLibraries.clear();
    LOGD("ElfUtils destroyed");
}

bool ElfUtils::loadLibrary(const std::string& path) {
    LOGD("Loading library: %s", path.c_str());
    
    // Check if already loaded
    if (isLibraryLoaded(path)) {
        LOGD("Library already loaded: %s", path.c_str());
        return true;
    }
    
    // Load using dlopen
    void* handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        LOGE("Failed to load library %s: %s", path.c_str(), dlerror());
        return false;
    }
    
    // Create ElfInfo
    auto elfInfo = std::make_shared<ElfInfo>();
    elfInfo->name = path;
    elfInfo->baseAddress = handle;
    elfInfo->size = 0; // Will be determined later if needed
    
    // Parse symbols if possible
    if (!parseElfHeader(path)) {
        LOGE("Failed to parse ELF header for: %s", path.c_str());
    }
    
    loadedLibraries[path] = elfInfo;
    LOGI("Library loaded successfully: %s", path.c_str());
    return true;
}

bool ElfUtils::unloadLibrary(const std::string& name) {
    auto it = loadedLibraries.find(name);
    if (it != loadedLibraries.end()) {
        if (it->second->baseAddress) {
            dlclose(it->second->baseAddress);
        }
        loadedLibraries.erase(it);
        LOGI("Library unloaded: %s", name.c_str());
        return true;
    }
    LOGE("Library not found for unloading: %s", name.c_str());
    return false;
}

void* ElfUtils::findSymbol(const std::string& libname, const std::string& symbol) {
    LOGD("Finding symbol %s in library %s", symbol.c_str(), libname.c_str());
    
    auto it = loadedLibraries.find(libname);
    if (it == loadedLibraries.end()) {
        LOGE("Library not loaded: %s", libname.c_str());
        return nullptr;
    }
    
    // Try cache first
    auto& symbols = it->second->symbols;
    auto symbolIt = symbols.find(symbol);
    if (symbolIt != symbols.end()) {
        LOGD("Symbol found in cache: %s", symbol.c_str());
        return symbolIt->second;
    }
    
    // Use dlsym
    void* addr = dlsym(it->second->baseAddress, symbol.c_str());
    if (addr) {
        // Cache the result
        symbols[symbol] = addr;
        LOGD("Symbol found via dlsym: %s at %p", symbol.c_str(), addr);
        return addr;
    }
    
    LOGE("Symbol not found: %s in %s", symbol.c_str(), libname.c_str());
    return nullptr;
}

void* ElfUtils::findSymbolInAll(const std::string& symbol) {
    LOGD("Finding symbol %s in all loaded libraries", symbol.c_str());
    
    // Search in all loaded libraries
    for (auto& pair : loadedLibraries) {
        void* addr = findSymbol(pair.first, symbol);
        if (addr) {
            LOGD("Symbol %s found in %s at %p", symbol.c_str(), pair.first.c_str(), addr);
            return addr;
        }
    }
    
    // Try default search with RTLD_DEFAULT
    void* addr = dlsym(RTLD_DEFAULT, symbol.c_str());
    if (addr) {
        LOGD("Symbol %s found via RTLD_DEFAULT at %p", symbol.c_str(), addr);
        return addr;
    }
    
    LOGE("Symbol not found in any library: %s", symbol.c_str());
    return nullptr;
}

bool ElfUtils::parseElfHeader(const std::string& path) {
    LOGD("Parsing ELF header for: %s", path.c_str());
    
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        LOGE("Failed to open file: %s", path.c_str());
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
    
    bool result = false;
    
    if (ehdr.e_ident[EI_CLASS] == ELFCLASS32) {
        LOGD("32-bit ELF detected");
        result = parseElf32((const char*)&ehdr, sizeof(ehdr), *loadedLibraries[path]);
    } else if (ehdr.e_ident[EI_CLASS] == ELFCLASS64) {
        LOGD("64-bit ELF detected");
        result = parseElf64((const char*)&ehdr, sizeof(ehdr), *loadedLibraries[path]);
    } else {
        LOGE("Unknown ELF class");
    }
    
    close(fd);
    return result;
}

bool ElfUtils::parseElf32(const char* data, size_t size, ElfInfo& info) {
    // Basic 32-bit ELF parsing
    LOGD("Parsing 32-bit ELF");
    // Implementation would go here for detailed parsing
    // For now, just return true as basic structure is set up
    return true;
}

bool ElfUtils::parseElf64(const char* data, size_t size, ElfInfo& info) {
    // Basic 64-bit ELF parsing
    LOGD("Parsing 64-bit ELF");
    // Implementation would go here for detailed parsing
    // For now, just return true as basic structure is set up
    return true;
}

std::vector<std::string> ElfUtils::getExportedSymbols(const std::string& libname) {
    std::vector<std::string> symbols;
    
    auto it = loadedLibraries.find(libname);
    if (it != loadedLibraries.end()) {
        for (const auto& symbolPair : it->second->symbols) {
            symbols.push_back(symbolPair.first);
        }
    }
    
    return symbols;
}

std::vector<std::string> ElfUtils::getImportedSymbols(const std::string& libname) {
    // Implementation would parse dynamic section for imported symbols
    // For now return empty vector
    return std::vector<std::string>();
}

void* ElfUtils::mapLibraryToMemory(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) return nullptr;
    
    // Get file size
    off_t size = lseek(fd, 0, SEEK_END);
    if (size <= 0) {
        close(fd);
        return nullptr;
    }
    
    // Map to memory
    void* addr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (addr == MAP_FAILED) {
        return nullptr;
    }
    
    return addr;
}

bool ElfUtils::unmapLibrary(void* addr, size_t size) {
    return munmap(addr, size) == 0;
}

bool ElfUtils::isLibraryLoaded(const std::string& name) {
    return loadedLibraries.find(name) != loadedLibraries.end();
}

std::vector<std::string> ElfUtils::getLoadedLibraries() {
    std::vector<std::string> libraries;
    for (const auto& pair : loadedLibraries) {
        libraries.push_back(pair.first);
    }
    return libraries;
}

void* ElfUtils::getLibraryBaseAddress(const std::string& name) {
    auto it = loadedLibraries.find(name);
    return (it != loadedLibraries.end()) ? it->second->baseAddress : nullptr;
}

size_t ElfUtils::getLibrarySize(const std::string& name) {
    auto it = loadedLibraries.find(name);
    return (it != loadedLibraries.end()) ? it->second->size : 0;
}