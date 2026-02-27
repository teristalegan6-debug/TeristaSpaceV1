#ifndef SYMBOL_RESOLVER_H
#define SYMBOL_RESOLVER_H

#include <string>
#include <vector>
#include <map>

struct SymbolInfo {
    std::string name;
    void* address;
    size_t size;
    std::string library;
    bool isFunction;
    bool isExported;
};

/**
 * Dynamic symbol resolution and library management
 */
class SymbolResolver {
public:
    SymbolResolver();
    ~SymbolResolver();
    
    // Symbol resolution
    void* resolveSymbol(const std::string& symbolName);
    void* resolveSymbol(const std::string& libraryName, const std::string& symbolName);
    std::vector<SymbolInfo> findSymbols(const std::string& pattern);
    
    // Library management
    bool scanLibrary(const std::string& libraryPath);
    bool scanAllLibraries();
    void clearCache();
    
    // Symbol information
    SymbolInfo getSymbolInfo(const std::string& symbolName);
    std::vector<std::string> getLibrarySymbols(const std::string& libraryName);
    std::vector<std::string> getLoadedLibraries();
    
    // Address utilities
    std::string getSymbolName(void* address);
    std::string getLibraryName(void* address);
    bool isValidSymbolAddress(void* address);
    
private:
    std::map<std::string, SymbolInfo> symbolCache;
    std::map<std::string, std::vector<std::string>> librarySymbols;
    std::vector<std::string> scannedLibraries;
    
    // Parsing functions
    bool parseElfSymbols(const std::string& libraryPath);
    bool parseDynamicSymbols(const std::string& libraryPath);
    void* dlsymWrapper(const std::string& libraryName, const std::string& symbolName);
    
    // Utility functions
    bool isSymbolVisible(const std::string& symbolName);
    std::string demangle(const std::string& mangledName);
    void addSymbol(const SymbolInfo& symbol);
};

#endif // SYMBOL_RESOLVER_H