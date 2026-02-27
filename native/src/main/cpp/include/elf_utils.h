#ifndef ELF_UTILS_H
#define ELF_UTILS_H

#include <string>
#include <vector>
#include <map>
#include <memory>

struct ElfInfo {
    std::string name;
    void* baseAddress;
    size_t size;
    std::map<std::string, void*> symbols;
};

/**
 * ELF file manipulation utilities
 */
class ElfUtils {
public:
    ElfUtils();
    ~ElfUtils();
    
    // Library loading
    bool loadLibrary(const std::string& path);
    bool unloadLibrary(const std::string& name);
    
    // Symbol resolution
    void* findSymbol(const std::string& libname, const std::string& symbol);
    void* findSymbolInAll(const std::string& symbol);
    
    // ELF parsing
    bool parseElfHeader(const std::string& path);
    std::vector<std::string> getExportedSymbols(const std::string& libname);
    std::vector<std::string> getImportedSymbols(const std::string& libname);
    
    // Memory mapping
    void* mapLibraryToMemory(const std::string& path);
    bool unmapLibrary(void* addr, size_t size);
    
    // Utility functions
    bool isLibraryLoaded(const std::string& name);
    std::vector<std::string> getLoadedLibraries();
    void* getLibraryBaseAddress(const std::string& name);
    size_t getLibrarySize(const std::string& name);
    
private:
    std::map<std::string, std::shared_ptr<ElfInfo>> loadedLibraries;
    
    bool parseElf32(const char* data, size_t size, ElfInfo& info);
    bool parseElf64(const char* data, size_t size, ElfInfo& info);
    void* findSymbolInElf(const ElfInfo& info, const std::string& symbol);
};

#endif // ELF_UTILS_H