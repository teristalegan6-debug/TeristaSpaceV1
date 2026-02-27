package com.terista.space.process;

import com.terista.space.reflection.ReflectionClass;
import java.util.concurrent.ConcurrentHashMap;

@ReflectionClass
public class VirtualProcessScheduler {
    
    private static final ConcurrentHashMap<Integer, VirtualTask> runningTasks = new ConcurrentHashMap<>();
    private static int nextTaskId = 1000;
    
    public static class VirtualTask {
        public int taskId;
        public String packageName;
        public Runnable task;
        public boolean isRunning;
        public long startTime;
        
        public VirtualTask(int taskId, String packageName, Runnable task) {
            this.taskId = taskId;
            this.packageName = packageName;
            this.task = task;
            this.isRunning = false;
            this.startTime = System.currentTimeMillis();
        }
    }
    
    public static int scheduleTask(String packageName, Runnable task) {
        int taskId = nextTaskId++;
        VirtualTask vTask = new VirtualTask(taskId, packageName, task);
        runningTasks.put(taskId, vTask);
        
        // Execute task in background thread
        new Thread(() -> {
            vTask.isRunning = true;
            try {
                vTask.task.run();
            } catch (Exception e) {
                android.util.Log.e("VirtualProcessScheduler", "Task execution failed", e);
            } finally {
                vTask.isRunning = false;
                runningTasks.remove(taskId);
            }
        }).start();
        
        return taskId;
    }
    
    public static boolean cancelTask(int taskId) {
        VirtualTask task = runningTasks.remove(taskId);
        if (task != null) {
            task.isRunning = false;
            return true;
        }
        return false;
    }
    
    public static VirtualTask getTask(int taskId) {
        return runningTasks.get(taskId);
    }
    
    public static void shutdown() {
        for (VirtualTask task : runningTasks.values()) {
            task.isRunning = false;
        }
        runningTasks.clear();
    }
}