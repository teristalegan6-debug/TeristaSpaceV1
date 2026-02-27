package com.terista.space.reflection;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Marks a field for reflection stub generation.
 * The annotation processor will generate field access stubs in BR* classes.
 */
@Retention(RetentionPolicy.CLASS)
@Target(ElementType.FIELD)
public @interface ReflectionField {
    /**
     * Custom name for the generated field accessor.
     * If not specified, uses the original field name.
     */
    String value() default "";
    
    /**
     * Whether this field is static.
     */
    boolean isStatic() default false;
    
    /**
     * Whether to generate getter method.
     */
    boolean getter() default true;
    
    /**
     * Whether to generate setter method.
     */
    boolean setter() default true;
}