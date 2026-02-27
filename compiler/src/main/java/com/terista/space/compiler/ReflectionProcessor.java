package com.terista.space.compiler;

import com.google.auto.service.AutoService;
import com.squareup.javapoet.*;
import com.terista.space.reflection.ReflectionClass;
import com.terista.space.reflection.ReflectionField;
import com.terista.space.reflection.ReflectionMethod;

import javax.annotation.processing.*;
import javax.lang.model.SourceVersion;
import javax.lang.model.element.*;
import javax.lang.model.type.TypeMirror;
import javax.lang.model.util.Elements;
import javax.lang.model.util.Types;
import javax.tools.Diagnostic;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.*;

/**
 * Annotation processor that generates BR* reflection stub classes.
 * Processes @ReflectionClass, @ReflectionMethod, and @ReflectionField annotations.
 */
@AutoService(Processor.class)
public class ReflectionProcessor extends AbstractProcessor {
    
    private Types typeUtils;
    private Elements elementUtils;
    private Filer filer;
    private Messager messager;
    
    @Override
    public synchronized void init(ProcessingEnvironment processingEnv) {
        super.init(processingEnv);
        typeUtils = processingEnv.getTypeUtils();
        elementUtils = processingEnv.getElementUtils();
        filer = processingEnv.getFiler();
        messager = processingEnv.getMessager();
    }
    
    @Override
    public Set<String> getSupportedAnnotationTypes() {
        Set<String> annotations = new LinkedHashSet<>();
        annotations.add(ReflectionClass.class.getCanonicalName());
        annotations.add(ReflectionMethod.class.getCanonicalName());
        annotations.add(ReflectionField.class.getCanonicalName());
        return annotations;
    }
    
    @Override
    public SourceVersion getSupportedSourceVersion() {
        return SourceVersion.latestSupported();
    }
    
    @Override
    public boolean process(Set<? extends TypeElement> annotations, RoundEnvironment roundEnv) {
        Map<TypeElement, ClassBuilder> classBuilders = new HashMap<>();
        
        // Process @ReflectionClass annotations
        for (Element element : roundEnv.getElementsAnnotatedWith(ReflectionClass.class)) {
            if (element.getKind() != ElementKind.CLASS) {
                messager.printMessage(Diagnostic.Kind.ERROR, 
                    "@ReflectionClass can only be applied to classes", element);
                continue;
            }
            
            TypeElement classElement = (TypeElement) element;
            ReflectionClass annotation = classElement.getAnnotation(ReflectionClass.class);
            
            String brClassName = getBRClassName(classElement, annotation);
            ClassBuilder builder = getOrCreateClassBuilder(classBuilders, classElement, brClassName);
            
            // Process methods and fields in the class
            for (Element enclosedElement : classElement.getEnclosedElements()) {
                if (enclosedElement.getKind() == ElementKind.METHOD) {
                    ReflectionMethod methodAnnotation = enclosedElement.getAnnotation(ReflectionMethod.class);
                    if (methodAnnotation != null) {
                        processMethod(builder, (ExecutableElement) enclosedElement, methodAnnotation);
                    }
                } else if (enclosedElement.getKind() == ElementKind.FIELD) {
                    ReflectionField fieldAnnotation = enclosedElement.getAnnotation(ReflectionField.class);
                    if (fieldAnnotation != null) {
                        processField(builder, (VariableElement) enclosedElement, fieldAnnotation);
                    }
                }
            }
        }
        
        // Generate BR classes
        for (ClassBuilder builder : classBuilders.values()) {
            try {
                builder.build().writeTo(filer);
            } catch (IOException e) {
                messager.printMessage(Diagnostic.Kind.ERROR, 
                    "Failed to generate BR class: " + e.getMessage());
            }
        }
        
        return true;
    }
    
    private String getBRClassName(TypeElement classElement, ReflectionClass annotation) {
        if (!annotation.name().isEmpty()) {
            return annotation.name();
        }
        return "BR" + classElement.getSimpleName().toString();
    }
    
    private ClassBuilder getOrCreateClassBuilder(Map<TypeElement, ClassBuilder> builders, 
                                               TypeElement classElement, String brClassName) {
        return builders.computeIfAbsent(classElement, k -> {
            String packageName = elementUtils.getPackageOf(classElement).getQualifiedName().toString();
            return new ClassBuilder(packageName, brClassName, classElement);
        });
    }
    
    private void processMethod(ClassBuilder builder, ExecutableElement methodElement, ReflectionMethod annotation) {
        String methodName = annotation.value().isEmpty() ? 
            methodElement.getSimpleName().toString() : annotation.value();
        
        builder.addMethod(methodName, methodElement, annotation.isStatic());
    }
    
    private void processField(ClassBuilder builder, VariableElement fieldElement, ReflectionField annotation) {
        String fieldName = annotation.value().isEmpty() ? 
            fieldElement.getSimpleName().toString() : annotation.value();
        
        builder.addField(fieldName, fieldElement, annotation.isStatic(), 
                        annotation.getter(), annotation.setter());
    }
    
    private static class ClassBuilder {
        private final String packageName;
        private final String className;
        private final TypeElement originalClass;
        private final TypeSpec.Builder classBuilder;
        private final Set<String> addedMethods = new HashSet<>();
        
        public ClassBuilder(String packageName, String className, TypeElement originalClass) {
            this.packageName = packageName;
            this.className = className;
            this.originalClass = originalClass;
            this.classBuilder = TypeSpec.classBuilder(className)
                .addModifiers(Modifier.PUBLIC, Modifier.FINAL);
            
            // Add static Class field
            classBuilder.addField(FieldSpec.builder(
                ParameterizedTypeName.get(ClassName.get(Class.class), WildcardTypeName.subtypeOf(Object.class)),
                "TYPE",
                Modifier.PUBLIC, Modifier.STATIC, Modifier.FINAL)
                .initializer("$T.class", TypeName.get(originalClass.asType()))
                .build());
        }
        
        public void addMethod(String methodName, ExecutableElement methodElement, boolean isStatic) {
            String uniqueName = methodName + "_" + methodElement.getParameters().size();
            if (addedMethods.contains(uniqueName)) {
                return;
            }
            addedMethods.add(uniqueName);
            
            MethodSpec.Builder methodBuilder = MethodSpec.methodBuilder(methodName)
                .addModifiers(Modifier.PUBLIC, Modifier.STATIC)
                .returns(TypeName.get(methodElement.getReturnType()));
            
            // Add parameters
            List<String> paramNames = new ArrayList<>();
            if (!isStatic) {
                methodBuilder.addParameter(TypeName.get(originalClass.asType()), "instance");
                paramNames.add("instance");
            }
            
            for (VariableElement param : methodElement.getParameters()) {
                String paramName = param.getSimpleName().toString();
                methodBuilder.addParameter(TypeName.get(param.asType()), paramName);
                paramNames.add(paramName);
            }
            
            // Generate method body
            CodeBlock.Builder bodyBuilder = CodeBlock.builder();
            bodyBuilder.beginControlFlow("try");
            
            // Get method reference
            bodyBuilder.addStatement("$T method = TYPE.getDeclaredMethod($S, $L)", 
                Method.class, methodElement.getSimpleName().toString(), 
                getParameterTypes(methodElement));
            bodyBuilder.addStatement("method.setAccessible(true)");
            
            // Invoke method
            String invokeParams = String.join(", ", paramNames);
            if (methodElement.getReturnType().getKind().name().equals("VOID")) {
                bodyBuilder.addStatement("method.invoke($L)", invokeParams);
            } else {
                bodyBuilder.addStatement("return ($T) method.invoke($L)", 
                    TypeName.get(methodElement.getReturnType()), invokeParams);
            }
            
            bodyBuilder.nextControlFlow("catch ($T e)", Exception.class);
            bodyBuilder.addStatement("throw new $T(e)", RuntimeException.class);
            bodyBuilder.endControlFlow();
            
            methodBuilder.addCode(bodyBuilder.build());
            classBuilder.addMethod(methodBuilder.build());
        }
        
        public void addField(String fieldName, VariableElement fieldElement, boolean isStatic, 
                           boolean addGetter, boolean addSetter) {
            if (addGetter) {
                addFieldGetter(fieldName, fieldElement, isStatic);
            }
            if (addSetter) {
                addFieldSetter(fieldName, fieldElement, isStatic);
            }
        }
        
        private void addFieldGetter(String fieldName, VariableElement fieldElement, boolean isStatic) {
            String methodName = "get" + capitalize(fieldName);
            
            MethodSpec.Builder methodBuilder = MethodSpec.methodBuilder(methodName)
                .addModifiers(Modifier.PUBLIC, Modifier.STATIC)
                .returns(TypeName.get(fieldElement.asType()));
            
            if (!isStatic) {
                methodBuilder.addParameter(TypeName.get(originalClass.asType()), "instance");
            }
            
            CodeBlock.Builder bodyBuilder = CodeBlock.builder();
            bodyBuilder.beginControlFlow("try");
            bodyBuilder.addStatement("$T field = TYPE.getDeclaredField($S)", Field.class, fieldElement.getSimpleName().toString());
            bodyBuilder.addStatement("field.setAccessible(true)");
            
            if (isStatic) {
                bodyBuilder.addStatement("return ($T) field.get(null)", TypeName.get(fieldElement.asType()));
            } else {
                bodyBuilder.addStatement("return ($T) field.get(instance)", TypeName.get(fieldElement.asType()));
            }
            
            bodyBuilder.nextControlFlow("catch ($T e)", Exception.class);
            bodyBuilder.addStatement("throw new $T(e)", RuntimeException.class);
            bodyBuilder.endControlFlow();
            
            methodBuilder.addCode(bodyBuilder.build());
            classBuilder.addMethod(methodBuilder.build());
        }
        
        private void addFieldSetter(String fieldName, VariableElement fieldElement, boolean isStatic) {
            String methodName = "set" + capitalize(fieldName);
            
            MethodSpec.Builder methodBuilder = MethodSpec.methodBuilder(methodName)
                .addModifiers(Modifier.PUBLIC, Modifier.STATIC)
                .returns(void.class);
            
            if (!isStatic) {
                methodBuilder.addParameter(TypeName.get(originalClass.asType()), "instance");
            }
            methodBuilder.addParameter(TypeName.get(fieldElement.asType()), "value");
            
            CodeBlock.Builder bodyBuilder = CodeBlock.builder();
            bodyBuilder.beginControlFlow("try");
            bodyBuilder.addStatement("$T field = TYPE.getDeclaredField($S)", Field.class, fieldElement.getSimpleName().toString());
            bodyBuilder.addStatement("field.setAccessible(true)");
            
            if (isStatic) {
                bodyBuilder.addStatement("field.set(null, value)");
            } else {
                bodyBuilder.addStatement("field.set(instance, value)");
            }
            
            bodyBuilder.nextControlFlow("catch ($T e)", Exception.class);
            bodyBuilder.addStatement("throw new $T(e)", RuntimeException.class);
            bodyBuilder.endControlFlow();
            
            methodBuilder.addCode(bodyBuilder.build());
            classBuilder.addMethod(methodBuilder.build());
        }
        
        private String getParameterTypes(ExecutableElement methodElement) {
            if (methodElement.getParameters().isEmpty()) {
                return "new Class[0]";
            }
            
            StringBuilder sb = new StringBuilder("new Class[]{");
            for (int i = 0; i < methodElement.getParameters().size(); i++) {
                if (i > 0) sb.append(", ");
                VariableElement param = methodElement.getParameters().get(i);
                sb.append(param.asType().toString()).append(".class");
            }
            sb.append("}");
            return sb.toString();
        }
        
        private String capitalize(String str) {
            return str.substring(0, 1).toUpperCase() + str.substring(1);
        }
        
        public JavaFile build() {
            return JavaFile.builder(packageName, classBuilder.build()).build();
        }
    }
}