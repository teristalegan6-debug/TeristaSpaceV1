package com.terista.space;

import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.terista.space.core.VirtualEngine;
import com.terista.space.core.VirtualApp;
import java.util.ArrayList;
import java.util.List;

/**
 * Main Activity for TeristaSpace - Virtual App Manager
 */
public class MainActivity extends AppCompatActivity {
    
    private static final String TAG = "MainActivity";
    
    private VirtualEngine virtualEngine;
    private RecyclerView recyclerView;
    private VirtualAppAdapter adapter;
    private FloatingActionButton fabAddApp;
    private List<VirtualApp> virtualApps = new ArrayList<>();
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        Log.i(TAG, "MainActivity onCreate");
        
        // Get virtual engine instance
        TeristaApplication app = (TeristaApplication) getApplication();
        virtualEngine = app.getVirtualEngine();
        
        if (!virtualEngine.isInitialized()) {
            Toast.makeText(this, "Virtual Engine not initialized", Toast.LENGTH_LONG).show();
            finish();
            return;
        }
        
        initializeUI();
        loadVirtualApps();
    }
    
    private void initializeUI() {
        recyclerView = findViewById(R.id.recyclerView);
        fabAddApp = findViewById(R.id.fabAddApp);
        
        // Setup RecyclerView
        recyclerView.setLayoutManager(new LinearLayoutManager(this));
        adapter = new VirtualAppAdapter(virtualApps, this::onAppClick);
        recyclerView.setAdapter(adapter);
        
        // Setup FAB
        fabAddApp.setOnClickListener(v -> {
            // Open app installation dialog/activity
            Toast.makeText(this, "Add Virtual App", Toast.LENGTH_SHORT).show();
            // TODO: Implement app installation UI
        });
    }
    
    private void loadVirtualApps() {
        try {
            List<VirtualApp> apps = virtualEngine.getInstalledApps();
            virtualApps.clear();
            virtualApps.addAll(apps);
            adapter.notifyDataSetChanged();
            
            Log.i(TAG, "Loaded " + apps.size() + " virtual apps");
        } catch (Exception e) {
            Log.e(TAG, "Failed to load virtual apps", e);
            Toast.makeText(this, "Failed to load virtual apps", Toast.LENGTH_SHORT).show();
        }
    }
    
    private void onAppClick(VirtualApp app) {
        Log.i(TAG, "App clicked: " + app.getPackageName());
        
        if (app.isRunning()) {
            // Stop app
            if (virtualEngine.stopVirtualApp(app.getPackageName())) {
                Toast.makeText(this, "App stopped: " + app.getAppName(), Toast.LENGTH_SHORT).show();
                loadVirtualApps(); // Refresh list
            } else {
                Toast.makeText(this, "Failed to stop app", Toast.LENGTH_SHORT).show();
            }
        } else {
            // Launch app
            if (virtualEngine.launchVirtualApp(app.getPackageName(), app.getUserId())) {
                Toast.makeText(this, "App launched: " + app.getAppName(), Toast.LENGTH_SHORT).show();
                loadVirtualApps(); // Refresh list
            } else {
                Toast.makeText(this, "Failed to launch app", Toast.LENGTH_SHORT).show();
            }
        }
    }
    
    @Override
    protected void onResume() {
        super.onResume();
        loadVirtualApps(); // Refresh when returning to activity
    }
}