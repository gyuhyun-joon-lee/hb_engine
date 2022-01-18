COMPILER = clang 
ARCHITECTURE = -march=armv8+simd #TODO(joon): -arch arm64 makes the code slower?
MAKEFLAGS += --silent

# -O0 = unoptimized, -O2 = compiler optimized
COMPILER_FLAGS = -g -Wall -O0 -std=c++11 -lm -pthread -D MEKA_DEBUG=1 -D MEKA_LLVM=1 -D MEKA_ARM=1 -D MEKA_VULKAN=0 -D MEKA_METAL=1 #to use any third part library, enable this -I../external
# to disable warning, prefix the name of the warning with no-
COMPILER_IGNORE_WARNINGS = -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-writable-strings -Wno-switch -Wno-objc-missing-super-calls -Wno-missing-braces -Wnonportable-include-path -Wno-uninitialized
MAIN_CODE_PATH = ../code

MACOS_BUILD_PATH = ../../build
MACOS_APP_NAME = meka.app
MACOS_EXE_PATH = $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/MacOS
MACOS_RESOURCE_PATH = $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/Resources
MACOS_FRAMEWORK = -framework Cocoa -framework CoreFoundation -framework CoreGraphics -framework CoreVideo -framework Metal -framework MetalKit -framework Foundation -framework QuartzCore -pthread

all : make_directory make_app compile_main cleanup
#all : make_directory make_app fox.dylib fox.app clean

make_directory : 
	mkdir -p $(MACOS_BUILD_PATH)

make_app : 
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/Resources
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/MacOS

# target:  dependency1 dependency2 ...
#       <tab> command
compile_main : $(MAIN_CODE_PATH)/macos_meka.mm
	$(COMPILER) $(ARCHITECTURE) $(COMPILER_FLAGS) $(COMPILER_IGNORE_WARNINGS)  $(MACOS_FRAMEWORK) -o $(MACOS_EXE_PATH)/meka $(MAIN_CODE_PATH)/macos_meka.mm 

#clean all the object files.
cleanup : 
	rm -rf *.o 
