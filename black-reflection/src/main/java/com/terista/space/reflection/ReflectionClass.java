package com.terista.space.reflection;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Marks a class for reflection stub generation.
 * The annotation processor will generate BR* classes for marked classes.
 */
@Retention(RetentionPolicy.CLASS)
@Target(ElementType.TYPE)
public @interface ReflectionClass {
    /**
     * The class to generate reflection stubs for.
     * If not specified, uses the annotated class itself.
     */
    Class<?> value() default Object.class;
    
    /**
     * Custom name for the generated BR class.
     * If not specified, uses BR + ClassName format.
     */
    String name() default "";
}