package com.terista.space.core;

import android.util.Log;
import com.terista.space.reflection.ReflectionClass;

import java.util.concurrent.ConcurrentHashMap;
import java.util.Set;
import java.util.HashSet;

@ReflectionClass
public class VirtualServiceManager {
    
    private static final String TAG = "VirtualServiceManager";
    private final VirtualEngine engine;
    private final ConcurrentHashMap<String, VirtualService> runningServices = new ConcurrentHashMap<>();
    
    public VirtualServiceManager(VirtualEngine engine) {
        this.engine = engine;
    }
    
    public boolean startService(String packageName, String serviceName) {
        try {
            VirtualService service = new VirtualService(packageName, serviceName);
            runningServices.put(serviceName, service);
            service.isRunning = true;
            
            Log.i(TAG, "Virtual service started: " + serviceName);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Failed to start service", e);
            return false;
        }
    }
    
    public boolean stopService(String serviceName) {
        VirtualService service = runningServices.remove(serviceName);
        if (service != null) {
            service.isRunning = false;
            Log.i(TAG, "Virtual service stopped: " + serviceName);
            return true;
        }
        return false;
    }
    
    public VirtualService getRunningService(String serviceName) {
        return runningServices.get(serviceName);
    }
    
    public Set<String> getRunningServices() {
        return new HashSet<>(runningServices.keySet());
    }
    
    public void shutdown() {
        for (VirtualService service : runningServices.values()) {
            service.isRunning = false;
        }
        runningServices.clear();
    }
    
    public static class VirtualService {
        public String packageName;
        public String serviceName;
        public int processId;
        public boolean isRunning;
        
        public VirtualService(String packageName, String serviceName) {
            this.packageName = packageName;
            this.serviceName = serviceName;
            this.isRunning = false;
        }
    }
}