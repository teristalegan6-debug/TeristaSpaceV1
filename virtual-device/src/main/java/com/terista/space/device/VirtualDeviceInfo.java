package com.terista.space.device;

import com.terista.space.reflection.ReflectionClass;
import java.util.concurrent.ConcurrentHashMap;

@ReflectionClass
public class VirtualDeviceInfo {
    
    private static final ConcurrentHashMap<String, String> deviceProperties = new ConcurrentHashMap<>();
    
    static {
        // Initialize fake device properties
        deviceProperties.put("android_id", "virtual_android_id_12345");
        deviceProperties.put("imei", "123456789012345");
        deviceProperties.put("serial", "VIRTUALDEVICE001");
        deviceProperties.put("brand", "TeristaVirtual");
        deviceProperties.put("model", "Virtual Phone");
        deviceProperties.put("manufacturer", "TeristaSpace");
    }
    
    public static String getDeviceProperty(String key) {
        return deviceProperties.get(key);
    }
    
    public static void setDeviceProperty(String key, String value) {
        deviceProperties.put(key, value);
    }
    
    public static String getAndroidId() {
        return getDeviceProperty("android_id");
    }
    
    public static String getIMEI() {
        return getDeviceProperty("imei");
    }
    
    public static String getSerial() {
        return getDeviceProperty("serial");
    }
}