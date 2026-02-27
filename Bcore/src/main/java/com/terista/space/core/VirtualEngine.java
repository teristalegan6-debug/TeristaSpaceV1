package com.terista.space.core;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.os.Process;
import android.util.Log;

import com.terista.space.native.NativeBridge;
import com.terista.space.reflection.ReflectionClass;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.List;
import java.util.ArrayList;

/**
 * Core virtual engine that manages the entire virtualization system.
 * This is the central hub that coordinates all virtual components.
 */
@ReflectionClass
public class VirtualEngine {
    
    private static final String TAG = "VirtualEngine";
    private static VirtualEngine instance;
    private static final Object LOCK = new Object();
    
    private Context hostContext;
    private NativeBridge nativeBridge;
    private VirtualAppManager appManager;
    private VirtualPackageManager packageManager;
    private VirtualActivityManager activityManager;
    private VirtualServiceManager serviceManager;
    private VirtualProcessManager processManager;
    
    private final ConcurrentHashMap<String, VirtualApp> virtualApps = new ConcurrentHashMap<>();
    private final AtomicInteger nextProcessId = new AtomicInteger(10000);
    
    private boolean initialized = false;
    private boolean hooksInstalled = false;
    
    private VirtualEngine() {}
    
    public static VirtualEngine getInstance() {
        if (instance == null) {
            synchronized (LOCK) {
                if (instance == null) {
                    instance = new VirtualEngine();
                }
            }
        }
        return instance;
    }
    
    /**
     * Initialize the virtual engine
     * @param context Host application context
     * @return true if initialization successful
     */
    public boolean initialize(Context context) {
        if (initialized) {
            Log.w(TAG, "VirtualEngine already initialized");
            return true;
        }
        
        Log.i(TAG, "Initializing VirtualEngine...");
        
        this.hostContext = context.getApplicationContext();
        
        try {
            // Initialize native bridge
            nativeBridge = NativeBridge.getInstance();
            if (!nativeBridge.safeInitialize(hostContext)) {
                Log.e(TAG, "Failed to initialize native bridge");
                return false;
            }
            
            // Initialize managers
            appManager = new VirtualAppManager(this);
            packageManager = new VirtualPackageManager(this);
            activityManager = new VirtualActivityManager(this);
            serviceManager = new VirtualServiceManager(this);
            processManager = new VirtualProcessManager(this);
            
            // Install hooks
            if (!installSystemHooks()) {
                Log.e(TAG, "Failed to install system hooks");
                return false;
            }
            
            initialized = true;
            Log.i(TAG, "VirtualEngine initialized successfully");
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "Failed to initialize VirtualEngine", e);
            return false;
        }
    }
    
    /**
     * Shutdown the virtual engine
     */
    public void shutdown() {
        if (!initialized) return;
        
        Log.i(TAG, "Shutting down VirtualEngine...");
        
        try {
            // Stop all virtual apps
            for (VirtualApp app : virtualApps.values()) {
                stopVirtualApp(app.getPackageName());
            }
            
            // Uninstall hooks
            uninstallSystemHooks();
            
            // Cleanup managers
            if (processManager != null) processManager.shutdown();
            if (serviceManager != null) serviceManager.shutdown();
            if (activityManager != null) activityManager.shutdown();
            if (packageManager != null) packageManager.shutdown();
            if (appManager != null) appManager.shutdown();
            
            // Cleanup native bridge
            if (nativeBridge != null) {
                nativeBridge.cleanup();
            }
            
            virtualApps.clear();
            initialized = false;
            
            Log.i(TAG, "VirtualEngine shutdown completed");
            
        } catch (Exception e) {
            Log.e(TAG, "Error during VirtualEngine shutdown", e);
        }
    }
    
    /**
     * Install a virtual app
     * @param apkPath Path to APK file
     * @param userId Virtual user ID
     * @return true if installation successful
     */
    public boolean installVirtualApp(String apkPath, int userId) {
        if (!initialized) {
            Log.e(TAG, "VirtualEngine not initialized");
            return false;
        }
        
        Log.i(TAG, "Installing virtual app: " + apkPath);
        
        try {
            return appManager.installApp(apkPath, userId);
        } catch (Exception e) {
            Log.e(TAG, "Failed to install virtual app", e);
            return false;
        }
    }
    
    /**
     * Uninstall a virtual app
     * @param packageName Package name to uninstall
     * @param userId Virtual user ID
     * @return true if uninstallation successful
     */
    public boolean uninstallVirtualApp(String packageName, int userId) {
        if (!initialized) {
            Log.e(TAG, "VirtualEngine not initialized");
            return false;
        }
        
        Log.i(TAG, "Uninstalling virtual app: " + packageName);
        
        try {
            stopVirtualApp(packageName);
            return appManager.uninstallApp(packageName, userId);
        } catch (Exception e) {
            Log.e(TAG, "Failed to uninstall virtual app", e);
            return false;
        }
    }
    
    /**
     * Launch a virtual app
     * @param packageName Package name to launch
     * @param userId Virtual user ID
     * @return true if launch successful
     */
    public boolean launchVirtualApp(String packageName, int userId) {
        if (!initialized) {
            Log.e(TAG, "VirtualEngine not initialized");
            return false;
        }
        
        Log.i(TAG, "Launching virtual app: " + packageName);
        
        try {
            VirtualApp app = virtualApps.get(packageName);
            if (app == null) {
                Log.e(TAG, "Virtual app not found: " + packageName);
                return false;
            }
            
            return activityManager.launchApp(app, userId);
        } catch (Exception e) {
            Log.e(TAG, "Failed to launch virtual app", e);
            return false;
        }
    }
    
    /**
     * Stop a virtual app
     * @param packageName Package name to stop
     * @return true if stop successful
     */
    public boolean stopVirtualApp(String packageName) {
        if (!initialized) return false;
        
        Log.i(TAG, "Stopping virtual app: " + packageName);
        
        try {
            VirtualApp app = virtualApps.get(packageName);
            if (app != null) {
                processManager.killAppProcesses(packageName);
                return true;
            }
            return false;
        } catch (Exception e) {
            Log.e(TAG, "Failed to stop virtual app", e);
            return false;
        }
    }
    
    /**
     * Get list of installed virtual apps
     * @return List of virtual apps
     */
    public List<VirtualApp> getInstalledApps() {
        return new ArrayList<>(virtualApps.values());
    }
    
    /**
     * Get virtual app by package name
     * @param packageName Package name
     * @return Virtual app or null if not found
     */
    public VirtualApp getVirtualApp(String packageName) {
        return virtualApps.get(packageName);
    }
    
    /**
     * Add virtual app to registry
     * @param app Virtual app to add
     */
    public void addVirtualApp(VirtualApp app) {
        virtualApps.put(app.getPackageName(), app);
    }
    
    /**
     * Remove virtual app from registry
     * @param packageName Package name to remove
     */
    public void removeVirtualApp(String packageName) {
        virtualApps.remove(packageName);
    }
    
    /**
     * Generate next virtual process ID
     * @return New process ID
     */
    public int generateProcessId() {
        return nextProcessId.getAndIncrement();
    }
    
    // Getters for managers
    public Context getHostContext() { return hostContext; }
    public NativeBridge getNativeBridge() { return nativeBridge; }
    public VirtualAppManager getAppManager() { return appManager; }
    public VirtualPackageManager getPackageManager() { return packageManager; }
    public VirtualActivityManager getActivityManager() { return activityManager; }
    public VirtualServiceManager getServiceManager() { return serviceManager; }
    public VirtualProcessManager getProcessManager() { return processManager; }
    
    public boolean isInitialized() { return initialized; }
    public boolean areHooksInstalled() { return hooksInstalled; }
    
    /**
     * Install system hooks for virtualization
     * @return true if successful
     */
    private boolean installSystemHooks() {
        if (hooksInstalled) return true;
        
        Log.i(TAG, "Installing system hooks...");
        
        try {
            // Install binder hooks
            if (!nativeBridge.hookBinder()) {
                Log.e(TAG, "Failed to install binder hooks");
                return false;
            }
            
            // Configure service filters
            setupServiceFilters();
            
            hooksInstalled = true;
            Log.i(TAG, "System hooks installed successfully");
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "Failed to install system hooks", e);
            return false;
        }
    }
    
    /**
     * Uninstall system hooks
     */
    private void uninstallSystemHooks() {
        if (!hooksInstalled) return;
        
        Log.i(TAG, "Uninstalling system hooks...");
        
        try {
            nativeBridge.unhookBinder();
            hooksInstalled = false;
            Log.i(TAG, "System hooks uninstalled");
        } catch (Exception e) {
            Log.e(TAG, "Failed to uninstall system hooks", e);
        }
    }
    
    /**
     * Setup service filters for binder interception
     */
    private void setupServiceFilters() {
        // Allow essential services
        nativeBridge.setBinderFilter("package", true);
        nativeBridge.setBinderFilter("activity", true);
        nativeBridge.setBinderFilter("window", true);
        nativeBridge.setBinderFilter("input", true);
        nativeBridge.setBinderFilter("power", true);
        
        // Block sensitive services for virtual apps
        nativeBridge.setBinderFilter("telephony.registry", false);
        nativeBridge.setBinderFilter("isms", false);
        nativeBridge.setBinderFilter("phone", false);
    }
}