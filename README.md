# TeristaSpace - Android Virtualization Engine

TeristaSpace is a complete Android virtualization engine that allows running multiple virtual Android environments within a single host application. It provides app isolation, virtual filesystem, fake device information, and comprehensive system service virtualization.

## Features

- **Virtual App Management**: Install, launch, and manage virtual Android applications
- **Complete Isolation**: Each virtual app runs in its own sandbox environment
- **Native Hooking Engine**: Advanced inline hooking system for ARM64/ARM32
- **Binder Interception**: System service call filtering and redirection
- **Virtual Device Info**: Fake IMEI, Android ID, serial numbers, and device properties
- **Virtual Filesystem**: Isolated file system for each virtual environment
- **Reflection Engine**: Advanced reflection stub generation system
- **Modern UI**: Material 3 design with intuitive app management interface

## Architecture

The project consists of multiple modules:

### Core Modules
- **app**: Main TeristaSpace application with Material 3 UI
- **Bcore**: Core virtual engine managing the entire virtualization system
- **native**: NDK-based hooking engine with C/C++ implementation
- **black-reflection**: Annotation-based reflection stub system
- **compiler**: Annotation processor for generating reflection stubs

### Virtual Environment Modules
- **sandbox**: Virtual runtime environment for apps
- **proxy**: Activity and service proxies for virtual apps
- **virtual-device**: Fake device information subsystem
- **virtual-services**: Virtual system service implementations
- **virtual-fs**: Virtual filesystem layer
- **virtual-process**: Process management and task scheduling

## Building

### Requirements
- Android Studio Arctic Fox or newer
- NDK 21 or newer
- Java 21
- Gradle 8.11.1
- Android SDK API 35

### Build Instructions

1. **Clone and Setup**:
   ```bash
   # Extract the project and navigate to directory
   cd TeristaSpace
   ```

2. **Build the project**:
   ```bash
   ./gradlew build
   ```

3. **Install APK**:
   ```bash
   ./gradlew installDebug
   ```

### Module Dependencies
```
app
├── Bcore (core engine)
├── native (hooking engine)  
├── sandbox (runtime)
├── proxy (activity/service proxy)
├── virtual-device (device info)
├── virtual-services (system services)
├── virtual-fs (filesystem)
└── virtual-process (process management)

Bcore
├── black-reflection (annotations)
└── native (native bridge)

Compiler
└── black-reflection (processes annotations)
```

## Technical Details

### Native Hooking Engine
- Inline ARM64/ARM32 assembly hooking
- ELF symbol resolution and manipulation
- Binder transaction interception
- Memory protection and allocation management

### Virtual Environment
- Complete app sandboxing
- System service virtualization
- File system redirection
- Process isolation and management

### Reflection System
- Compile-time reflection stub generation
- Annotation-based API (`@ReflectionClass`, `@ReflectionMethod`, `@ReflectionField`)
- Automatic proxy class generation with JavaPoet

## Security Notes

This is a research and development project demonstrating Android virtualization techniques. Use responsibly and in accordance with applicable laws and platform policies.

## License

This project is for educational and research purposes. The code is provided as-is for learning about Android internals and virtualization techniques.

## Contributing

This is a demonstration project showcasing a complete virtual Android engine implementation. The codebase serves as an educational resource for understanding Android system-level programming and virtualization concepts.