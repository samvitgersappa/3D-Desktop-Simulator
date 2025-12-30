#!/bin/bash

# Build script for macOS
echo "Building 3D-Room-Simulation for macOS..."

# Check if clang++ is installed
if ! command -v clang++ &> /dev/null; then
    echo "Error: clang++ is not installed. Please install Xcode Command Line Tools."
    exit 1
fi

# Compile
# -framework OpenGL: Standard OpenGL
# -framework GLUT: Standard GLUT
# -framework OpenAL: Audio
# -DUSE_OPENAL: Activates the audio code in audio.cpp
# -Wno-deprecated-declarations: Suppress warnings about OpenGL/OpenAL deprecation on Mac
# Use /usr/bin/clang++ to ensure we use the system compiler which should have correct SDK paths
/usr/bin/clang++ -o DesktopSimulation main.cpp audio.cpp \
    -framework OpenGL \
    -framework GLUT \
    -framework OpenAL \
    -DUSE_OPENAL \
    -Wno-deprecated-declarations \
    -std=c++11

if [ $? -eq 0 ]; then
    echo "Build successful! Executable is 'DesktopSimulation'"
    echo "To run, use: ./DesktopSimulation"
else
    echo "Build failed."
    exit 1
fi
