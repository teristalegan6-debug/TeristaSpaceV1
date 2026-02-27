package com.terista.space.core;

import android.util.Log;
import com.terista.space.reflection.ReflectionClass;

import java.util.concurrent.ConcurrentHashMap;
import java.util.Set;
import java.util.HashSet;

@ReflectionClass
public class VirtualProcessManager {
    
    private static final String TAG = "VirtualProcessManager";
    private final VirtualEngine engine;
    private final ConcurrentHashMap<Integer, VirtualProcess> runningProcesses = new ConcurrentHashMap<>();
    
    public VirtualProcessManager(VirtualEngine engine) {
        this.engine = engine;
    }
    
    public VirtualProcess createProcess(String packageName, int userId) {
        int processId = engine.generateProcessId();
        VirtualProcess process = new VirtualProcess(processId, packageName, userId);
        runningProcesses.put(processId, process);
        
        Log.i(TAG, "Virtual process created: PID=" + processId + ", package=" + packageName);
        return process;
    }
    
    public boolean killProcess(int processId) {
        VirtualProcess process = runningProcesses.remove(processId);
        if (process != null) {
            process.isAlive = false;
            engine.getNativeBridge().killVirtualProcess(processId);
            Log.i(TAG, "Virtual process killed: PID=" + processId);
            return true;
        }
        return false;
    }
    
    public void killAppProcesses(String packageName) {
        Set<Integer> toKill = new HashSet<>();
        for (VirtualProcess process : runningProcesses.values()) {
            if (packageName.equals(process.packageName)) {
                toKill.add(process.processId);
            }
        }
        
        for (Integer pid : toKill) {
            killProcess(pid);
        }
    }
    
    public VirtualProcess getProcess(int processId) {
        return runningProcesses.get(processId);
    }
    
    public Set<VirtualProcess> getAllProcesses() {
        return new HashSet<>(runningProcesses.values());
    }
    
    public void shutdown() {
        for (VirtualProcess process : runningProcesses.values()) {
            process.isAlive = false;
        }
        runningProcesses.clear();
    }
    
    public static class VirtualProcess {
        public int processId;
        public String packageName;
        public int userId;
        public long startTime;
        public boolean isAlive;
        
        public VirtualProcess(int processId, String packageName, int userId) {
            this.processId = processId;
            this.packageName = packageName;
            this.userId = userId;
            this.startTime = System.currentTimeMillis();
            this.isAlive = true;
        }
    }
}