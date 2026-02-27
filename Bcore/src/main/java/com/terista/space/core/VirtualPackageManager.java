package com.terista.space.core;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.util.Log;
import com.terista.space.reflection.ReflectionClass;

import java.util.concurrent.ConcurrentHashMap;
import java.util.List;
import java.util.ArrayList;

@ReflectionClass
public class VirtualPackageManager {
    
    private static final String TAG = "VirtualPackageManager";
    private final VirtualEngine engine;
    private final ConcurrentHashMap<String, PackageInfo> virtualPackages = new ConcurrentHashMap<>();
    
    public VirtualPackageManager(VirtualEngine engine) {
        this.engine = engine;
    }
    
    public void addVirtualPackage(String packageName, PackageInfo packageInfo) {
        virtualPackages.put(packageName, packageInfo);
    }
    
    public PackageInfo getVirtualPackageInfo(String packageName) {
        return virtualPackages.get(packageName);
    }
    
    public List<PackageInfo> getAllVirtualPackages() {
        return new ArrayList<>(virtualPackages.values());
    }
    
    public boolean isVirtualPackage(String packageName) {
        return virtualPackages.containsKey(packageName);
    }
    
    public void removeVirtualPackage(String packageName) {
        virtualPackages.remove(packageName);
    }
    
    public void shutdown() {
        virtualPackages.clear();
    }
}