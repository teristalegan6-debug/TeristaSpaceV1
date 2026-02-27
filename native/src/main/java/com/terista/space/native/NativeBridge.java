package com.terista.space.native;

import android.content.Context;

/**
 * JNI bridge to the native TeristaSpace hooking engine.
 * Provides interface to native hooking, memory management, and process control.
 */
public class NativeBridge {
    
    private static final String NATIVE_LIB_NAME = "teristanative";
    private static boolean isLoaded = false;
    private static NativeBridge instance;
    
    static {
        try {
            System.loadLibrary(NATIVE_LIB_NAME);
            isLoaded = true;
        } catch (UnsatisfiedLinkError e) {
            android.util.Log.e("NativeBridge", "Failed to load native library: " + NATIVE_LIB_NAME, e);
            isLoaded = false;
        }
    }
    
    private NativeBridge() {}
    
    public static synchronized NativeBridge getInstance() {
        if (instance == null) {
            instance = new NativeBridge();
        }
        return instance;
    }
    
    public static boolean isNativeLibraryLoaded() {
        return isLoaded;
    }
    
    // Native method declarations
    
    /**
     * Initialize the native hooking engine
     * @param context Application context
     * @return true if initialization successful
     */
    public native boolean initialize(Context context);
    
    /**
     * Cleanup and shutdown native engine
     */
    public native void cleanup();
    
    /**
     * Install an inline hook for a symbol
     * @param symbol Symbol name to hook
     * @param replacement Replacement function address
     * @param backup Pointer to store original function address
     * @return true if hook installed successfully
     */
    public native boolean installHook(String symbol, long replacement, long backup);
    
    /**
     * Uninstall a previously installed hook
     * @param symbol Symbol name to unhook
     * @return true if hook uninstalled successfully
     */
    public native boolean uninstallHook(String symbol);
    
    /**
     * Find symbol address in specified library
     * @param libname Library name
     * @param symbol Symbol name
     * @return Symbol address or 0 if not found
     */
    public native long findSymbol(String libname, String symbol);
    
    /**
     * Load a native library for hooking
     * @param path Library path
     * @return true if library loaded successfully
     */
    public native boolean loadLibrary(String path);
    
    /**
     * Install Binder IPC hooks
     * @return true if hooks installed successfully
     */
    public native boolean hookBinder();
    
    /**
     * Uninstall Binder IPC hooks
     */
    public native void unhookBinder();
    
    /**
     * Set service filter for Binder interception
     * @param serviceName Service name to filter
     * @param allow true to allow, false to block
     * @return true if filter set successfully
     */
    public native boolean setBinderFilter(String serviceName, boolean allow);
    
    /**
     * Create a virtual process
     * @param packageName Package name
     * @param userId User ID
     * @return true if process created successfully
     */
    public native boolean createVirtualProcess(String packageName, int userId);
    
    /**
     * Kill a virtual process
     * @param pid Process ID
     * @return true if process killed successfully
     */
    public native boolean killVirtualProcess(int pid);
    
    /**
     * Protect memory region
     * @param addr Memory address
     * @param size Memory size
     * @param prot Protection flags
     * @return true if memory protected successfully
     */
    public native boolean protectMemory(long addr, long size, int prot);
    
    /**
     * Allocate memory
     * @param size Size to allocate
     * @return Memory address or 0 if allocation failed
     */
    public native long allocateMemory(long size);
    
    /**
     * Free allocated memory
     * @param addr Memory address
     * @param size Memory size
     * @return true if memory freed successfully
     */
    public native boolean freeMemory(long addr, long size);
    
    // Helper methods
    
    /**
     * Initialize native bridge with error handling
     * @param context Application context
     * @return true if successful
     */
    public boolean safeInitialize(Context context) {
        if (!isLoaded) {
            android.util.Log.e("NativeBridge", "Native library not loaded");
            return false;
        }
        
        try {
            return initialize(context);
        } catch (Exception e) {
            android.util.Log.e("NativeBridge", "Failed to initialize native bridge", e);
            return false;
        }
    }
    
    /**
     * Install hook with error handling
     * @param symbol Symbol to hook
     * @param replacement Replacement function
     * @return true if successful
     */
    public boolean safeInstallHook(String symbol, long replacement) {
        if (!isLoaded) return false;
        
        try {
            return installHook(symbol, replacement, 0);
        } catch (Exception e) {
            android.util.Log.e("NativeBridge", "Failed to install hook for: " + symbol, e);
            return false;
        }
    }
    
    /**
     * Find symbol with error handling
     * @param libname Library name
     * @param symbol Symbol name
     * @return Symbol address or 0
     */
    public long safeFindSymbol(String libname, String symbol) {
        if (!isLoaded) return 0;
        
        try {
            return findSymbol(libname, symbol);
        } catch (Exception e) {
            android.util.Log.e("NativeBridge", "Failed to find symbol: " + symbol, e);
            return 0;
        }
    }
    
    /**
     * Get native library status
     * @return Status string
     */
    public String getStatus() {
        if (!isLoaded) {
            return "Native library not loaded";
        }
        return "Native library loaded successfully";
    }
}