package com.terista.space.core;

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import java.io.Serializable;

/**
 * Represents a virtual app installed in the virtual environment
 */
public class VirtualApp implements Serializable {
    
    private String packageName;
    private String appName;
    private String apkPath;
    private int userId;
    private long installTime;
    private ApplicationInfo applicationInfo;
    private PackageInfo packageInfo;
    private int versionCode;
    private String versionName;
    private boolean isRunning;
    private int processId;
    
    public VirtualApp(String packageName, String apkPath, int userId) {
        this.packageName = packageName;
        this.apkPath = apkPath;
        this.userId = userId;
        this.installTime = System.currentTimeMillis();
        this.isRunning = false;
        this.processId = -1;
    }
    
    // Getters and setters
    public String getPackageName() { return packageName; }
    public void setPackageName(String packageName) { this.packageName = packageName; }
    
    public String getAppName() { return appName; }
    public void setAppName(String appName) { this.appName = appName; }
    
    public String getApkPath() { return apkPath; }
    public void setApkPath(String apkPath) { this.apkPath = apkPath; }
    
    public int getUserId() { return userId; }
    public void setUserId(int userId) { this.userId = userId; }
    
    public long getInstallTime() { return installTime; }
    public void setInstallTime(long installTime) { this.installTime = installTime; }
    
    public ApplicationInfo getApplicationInfo() { return applicationInfo; }
    public void setApplicationInfo(ApplicationInfo applicationInfo) { this.applicationInfo = applicationInfo; }
    
    public PackageInfo getPackageInfo() { return packageInfo; }
    public void setPackageInfo(PackageInfo packageInfo) { this.packageInfo = packageInfo; }
    
    public int getVersionCode() { return versionCode; }
    public void setVersionCode(int versionCode) { this.versionCode = versionCode; }
    
    public String getVersionName() { return versionName; }
    public void setVersionName(String versionName) { this.versionName = versionName; }
    
    public boolean isRunning() { return isRunning; }
    public void setRunning(boolean running) { isRunning = running; }
    
    public int getProcessId() { return processId; }
    public void setProcessId(int processId) { this.processId = processId; }
    
    @Override
    public String toString() {
        return "VirtualApp{" +
                "packageName='" + packageName + '\'' +
                ", appName='" + appName + '\'' +
                ", userId=" + userId +
                ", isRunning=" + isRunning +
                '}';
    }
}