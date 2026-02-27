package com.terista.space.core;

import android.content.Intent;
import android.util.Log;
import com.terista.space.reflection.ReflectionClass;

import java.util.concurrent.ConcurrentHashMap;

@ReflectionClass
public class VirtualActivityManager {
    
    private static final String TAG = "VirtualActivityManager";
    private final VirtualEngine engine;
    private final ConcurrentHashMap<String, VirtualActivity> runningActivities = new ConcurrentHashMap<>();
    
    public VirtualActivityManager(VirtualEngine engine) {
        this.engine = engine;
    }
    
    public boolean launchApp(VirtualApp app, int userId) {
        try {
            Log.i(TAG, "Launching virtual app: " + app.getPackageName());
            
            // Create virtual process
            int processId = engine.generateProcessId();
            if (!engine.getNativeBridge().createVirtualProcess(app.getPackageName(), userId)) {
                Log.e(TAG, "Failed to create virtual process");
                return false;
            }
            
            app.setProcessId(processId);
            app.setRunning(true);
            
            Log.i(TAG, "Virtual app launched successfully: " + app.getPackageName());
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "Failed to launch virtual app", e);
            return false;
        }
    }
    
    public void addRunningActivity(String token, VirtualActivity activity) {
        runningActivities.put(token, activity);
    }
    
    public VirtualActivity getRunningActivity(String token) {
        return runningActivities.get(token);
    }
    
    public void removeRunningActivity(String token) {
        runningActivities.remove(token);
    }
    
    public void shutdown() {
        runningActivities.clear();
    }
    
    public static class VirtualActivity {
        public String packageName;
        public String className;
        public Intent intent;
        public int processId;
        public boolean isRunning;
        
        public VirtualActivity(String packageName, String className) {
            this.packageName = packageName;
            this.className = className;
            this.isRunning = false;
        }
    }
}