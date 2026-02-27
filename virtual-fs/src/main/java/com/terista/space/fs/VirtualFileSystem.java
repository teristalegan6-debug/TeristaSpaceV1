package com.terista.space.fs;

import com.terista.space.reflection.ReflectionClass;
import java.io.File;
import java.util.concurrent.ConcurrentHashMap;

@ReflectionClass
public class VirtualFileSystem {
    
    private static final ConcurrentHashMap<String, VirtualFile> virtualFiles = new ConcurrentHashMap<>();
    private static final String VIRTUAL_ROOT = "/virtual_fs";
    
    public static class VirtualFile {
        public String virtualPath;
        public String realPath;
        public boolean isDirectory;
        public long size;
        public long lastModified;
        
        public VirtualFile(String virtualPath, String realPath, boolean isDirectory) {
            this.virtualPath = virtualPath;
            this.realPath = realPath;
            this.isDirectory = isDirectory;
            this.lastModified = System.currentTimeMillis();
        }
    }
    
    public static boolean createVirtualPath(String virtualPath, String realPath) {
        File realFile = new File(realPath);
        VirtualFile vFile = new VirtualFile(virtualPath, realPath, realFile.isDirectory());
        if (realFile.exists()) {
            vFile.size = realFile.length();
            vFile.lastModified = realFile.lastModified();
        }
        virtualFiles.put(virtualPath, vFile);
        return true;
    }
    
    public static VirtualFile getVirtualFile(String virtualPath) {
        return virtualFiles.get(virtualPath);
    }
    
    public static String mapVirtualToReal(String virtualPath) {
        VirtualFile vFile = virtualFiles.get(virtualPath);
        return vFile != null ? vFile.realPath : null;
    }
    
    public static boolean exists(String virtualPath) {
        VirtualFile vFile = virtualFiles.get(virtualPath);
        if (vFile != null) {
            return new File(vFile.realPath).exists();
        }
        return false;
    }
    
    public static boolean delete(String virtualPath) {
        VirtualFile vFile = virtualFiles.remove(virtualPath);
        if (vFile != null) {
            return new File(vFile.realPath).delete();
        }
        return false;
    }
}