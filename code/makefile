COMPILER = clang 
#ARCHITECTURE = -arch arm64 #TODO(joon): -arch arm64 makes the code slower? #TODO(joon) -march=armv8+simd causes error?
ARCHITECTURE = -march=armv8+simd 
#cpu ?
MAKEFLAGS += --silent

MAIN_CODE_PATH = ../code
MACOS_BUILD_PATH = ../build
MACOS_APP_NAME = HB.app
MACOS_EXE_PATH = $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/MacOS
MACOS_RESOURCE_PATH = $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/Resources
MACOS_FRAMEWORK = -framework Cocoa -framework CoreFoundation -framework CoreGraphics -framework CoreVideo -framework Metal -framework MetalKit

# -O0 = unoptimized, -O2 = compiler optimized
COMPILER_FLAGS = -g -Wall -O2 -std=c++11 -lstdc++ -lm -pthread -D HB_DEBUG=1 -D HB_SLOW=1 -D HB_ARM=1 -D HB_X86_X64=0 -D HB_LLVM=1 -D HB_MSVC=0 -D HB_WINDOWS=0 -D HB_MACOS=1 -D HB_LINUX=0 -D HB_VULKAN=0 -D HB_METAL=1
# This is a nightmare.. :(
# to disable warning, prefix the name of the warning with no-
COMPILER_IGNORE_WARNINGS = -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-writable-strings -Wno-switch -Wno-objc-missing-super-calls -Wno-missing-braces -Wnonportable-include-path -Wno-uninitialized -Wno-nonportable-include-path -Wno-tautological-bitwise-compare -Wno-unused-but-set-variable

all : make_directory make_app compile_test_asset_packer compile_main create_lock compile_game delete_lock cleanup

make_directory : 
	mkdir -p $(MACOS_BUILD_PATH)

make_app : 
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/Resources
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/MacOS

compile_test_asset_packer : $(MAIN_CODE_PATH)/test_asset_packer.mm
	#$(COMPILER) $(ARCHITECTURE) $(COMPILER_FLAGS) $(COMPILER_IGNORE_WARNINGS) $(MACOS_FRAMEWORK) -o $(MACOS_EXE_PATH)/HB $(MAIN_CODE_PATH)/test_asset_packer.mm

# target:  dependency1 dependency2 ...
#       <tab> command
compile_main : $(MAIN_CODE_PATH)/macos_hb.mm
	$(COMPILER) $(ARCHITECTURE) $(COMPILER_FLAGS) $(COMPILER_IGNORE_WARNINGS) $(MACOS_FRAMEWORK) -o $(MACOS_EXE_PATH)/HB $(MAIN_CODE_PATH)/macos_hb.mm 

create_lock : 
	touch $(MACOS_EXE_PATH)/lock.tmp

# TODO(joon) map file is not being generated correctly...
compile_game : 
	$(COMPILER) $(ARCHITECTURE)  -Wl,-map,$(MACOS_EXE_PATH)/output.map $(COMPILER_FLAGS) -D debug_records=game_debug_records -dynamiclib $(COMPILER_IGNORE_WARNINGS) -o $(MACOS_EXE_PATH)/hb.dylib $(MAIN_CODE_PATH)/hb.cpp 

delete_lock : 
	rm $(MACOS_EXE_PATH)/lock.tmp

#clean all the object files.
cleanup : 
	 rm -rf *.o 
