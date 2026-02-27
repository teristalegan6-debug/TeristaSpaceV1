package com.terista.space.reflection;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Marks a method for reflection stub generation.
 * The annotation processor will generate stub methods in BR* classes.
 */
@Retention(RetentionPolicy.CLASS)
@Target(ElementType.METHOD)
public @interface ReflectionMethod {
    /**
     * Custom name for the generated method stub.
     * If not specified, uses the original method name.
     */
    String value() default "";
    
    /**
     * Whether this method is static.
     */
    boolean isStatic() default false;
    
    /**
     * Parameter types for method signature resolution.
     * Used when method is overloaded.
     */
    Class<?>[] parameterTypes() default {};
}