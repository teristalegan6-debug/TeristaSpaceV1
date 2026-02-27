package com.terista.space.core;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.util.Log;

import java.io.File;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Virtual App Manager - handles virtual app installation and management
 */
public class VirtualAppManager {
    
    private static final String TAG = "VirtualAppManager";
    private final VirtualEngine engine;
    private final ConcurrentHashMap<String, VirtualApp> installedApps = new ConcurrentHashMap<>();
    
    public VirtualAppManager(VirtualEngine engine) {
        this.engine = engine;
    }
    
    public boolean installApp(String apkPath, int userId) {
        try {
            File apkFile = new File(apkPath);
            if (!apkFile.exists()) {
                Log.e(TAG, "APK file not found: " + apkPath);
                return false;
            }
            
            // Parse APK to get package info
            PackageManager pm = engine.getHostContext().getPackageManager();
            PackageInfo packageInfo = pm.getPackageArchiveInfo(apkPath, PackageManager.GET_ACTIVITIES | PackageManager.GET_SERVICES);
            
            if (packageInfo == null) {
                Log.e(TAG, "Failed to parse APK: " + apkPath);
                return false;
            }
            
            String packageName = packageInfo.packageName;
            
            // Create virtual app
            VirtualApp virtualApp = new VirtualApp(packageName, apkPath, userId);
            virtualApp.setPackageInfo(packageInfo);
            virtualApp.setApplicationInfo(packageInfo.applicationInfo);
            virtualApp.setVersionCode(packageInfo.versionCode);
            virtualApp.setVersionName(packageInfo.versionName);
            
            if (packageInfo.applicationInfo != null && packageInfo.applicationInfo.loadLabel(pm) != null) {
                virtualApp.setAppName(packageInfo.applicationInfo.loadLabel(pm).toString());
            } else {
                virtualApp.setAppName(packageName);
            }
            
            // Install in virtual environment
            installedApps.put(packageName, virtualApp);
            engine.addVirtualApp(virtualApp);
            
            Log.i(TAG, "Virtual app installed: " + packageName);
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "Failed to install virtual app", e);
            return false;
        }
    }
    
    public boolean uninstallApp(String packageName, int userId) {
        try {
            VirtualApp app = installedApps.remove(packageName);
            if (app != null) {
                engine.removeVirtualApp(packageName);
                Log.i(TAG, "Virtual app uninstalled: " + packageName);
                return true;
            }
            return false;
        } catch (Exception e) {
            Log.e(TAG, "Failed to uninstall virtual app", e);
            return false;
        }
    }
    
    public VirtualApp getApp(String packageName) {
        return installedApps.get(packageName);
    }
    
    public void shutdown() {
        installedApps.clear();
    }
}