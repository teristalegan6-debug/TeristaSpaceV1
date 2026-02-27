package com.terista.space;

import android.app.Application;
import android.util.Log;
import com.terista.space.core.VirtualEngine;

/**
 * TeristaSpace Application class - initializes the virtual engine
 */
public class TeristaApplication extends Application {
    
    private static final String TAG = "TeristaApplication";
    private VirtualEngine virtualEngine;
    
    @Override
    public void onCreate() {
        super.onCreate();
        
        Log.i(TAG, "Initializing TeristaSpace Application...");
        
        // Initialize virtual engine
        virtualEngine = VirtualEngine.getInstance();
        
        if (!virtualEngine.initialize(this)) {
            Log.e(TAG, "Failed to initialize Virtual Engine");
        } else {
            Log.i(TAG, "Virtual Engine initialized successfully");
        }
    }
    
    @Override
    public void onTerminate() {
        super.onTerminate();
        
        if (virtualEngine != null) {
            virtualEngine.shutdown();
            Log.i(TAG, "Virtual Engine shutdown completed");
        }
    }
    
    public VirtualEngine getVirtualEngine() {
        return virtualEngine;
    }
}