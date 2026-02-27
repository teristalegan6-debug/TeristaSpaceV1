# Add project specific ProGuard rules here.

# Keep all virtual engine classes
-keep class com.terista.space.** { *; }

# Keep native methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep reflection annotations
-keep @com.terista.space.reflection.ReflectionClass class * { *; }
-keep class * {
    @com.terista.space.reflection.ReflectionMethod *;
}
-keep class * {
    @com.terista.space.reflection.ReflectionField *;
}

