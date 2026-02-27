package com.terista.space;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.terista.space.core.VirtualApp;
import java.util.List;
import java.util.function.Consumer;

/**
 * RecyclerView adapter for displaying virtual apps
 */
public class VirtualAppAdapter extends RecyclerView.Adapter<VirtualAppAdapter.ViewHolder> {
    
    private final List<VirtualApp> apps;
    private final Consumer<VirtualApp> onItemClick;
    
    public VirtualAppAdapter(List<VirtualApp> apps, Consumer<VirtualApp> onItemClick) {
        this.apps = apps;
        this.onItemClick = onItemClick;
    }
    
    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.item_virtual_app, parent, false);
        return new ViewHolder(view);
    }
    
    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        VirtualApp app = apps.get(position);
        
        holder.appName.setText(app.getAppName() != null ? app.getAppName() : app.getPackageName());
        holder.packageName.setText(app.getPackageName());
        holder.status.setText(app.isRunning() ? "Running" : "Stopped");
        holder.userId.setText("User: " + app.getUserId());
        
        // Set status color
        if (app.isRunning()) {
            holder.status.setTextColor(holder.itemView.getContext().getColor(android.R.color.holo_green_dark));
        } else {
            holder.status.setTextColor(holder.itemView.getContext().getColor(android.R.color.holo_red_dark));
        }
        
        // Set click listener
        holder.itemView.setOnClickListener(v -> {
            if (onItemClick != null) {
                onItemClick.accept(app);
            }
        });
    }
    
    @Override
    public int getItemCount() {
        return apps.size();
    }
    
    public static class ViewHolder extends RecyclerView.ViewHolder {
        public final TextView appName;
        public final TextView packageName;
        public final TextView status;
        public final TextView userId;
        
        public ViewHolder(@NonNull View itemView) {
            super(itemView);
            appName = itemView.findViewById(R.id.textAppName);
            packageName = itemView.findViewById(R.id.textPackageName);
            status = itemView.findViewById(R.id.textStatus);
            userId = itemView.findViewById(R.id.textUserId);
        }
    }
}