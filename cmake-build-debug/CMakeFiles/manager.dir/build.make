# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake

# The command to remove a file.
RM = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/zeroisone/projects/intellikeeper-hardware

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/zeroisone/projects/intellikeeper-hardware/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/manager.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/manager.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/manager.dir/flags.make

CMakeFiles/manager.dir/node/manager.o: CMakeFiles/manager.dir/flags.make
CMakeFiles/manager.dir/node/manager.o: ../node/manager.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zeroisone/projects/intellikeeper-hardware/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/manager.dir/node/manager.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/manager.dir/node/manager.o -c /Users/zeroisone/projects/intellikeeper-hardware/node/manager.cpp

CMakeFiles/manager.dir/node/manager.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/manager.dir/node/manager.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/zeroisone/projects/intellikeeper-hardware/node/manager.cpp > CMakeFiles/manager.dir/node/manager.i

CMakeFiles/manager.dir/node/manager.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/manager.dir/node/manager.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/zeroisone/projects/intellikeeper-hardware/node/manager.cpp -o CMakeFiles/manager.dir/node/manager.s

# Object files for target manager
manager_OBJECTS = \
"CMakeFiles/manager.dir/node/manager.o"

# External object files for target manager
manager_EXTERNAL_OBJECTS =

libmanager.a: CMakeFiles/manager.dir/node/manager.o
libmanager.a: CMakeFiles/manager.dir/build.make
libmanager.a: CMakeFiles/manager.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/zeroisone/projects/intellikeeper-hardware/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libmanager.a"
	$(CMAKE_COMMAND) -P CMakeFiles/manager.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/manager.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/manager.dir/build: libmanager.a

.PHONY : CMakeFiles/manager.dir/build

CMakeFiles/manager.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/manager.dir/cmake_clean.cmake
.PHONY : CMakeFiles/manager.dir/clean

CMakeFiles/manager.dir/depend:
	cd /Users/zeroisone/projects/intellikeeper-hardware/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/zeroisone/projects/intellikeeper-hardware /Users/zeroisone/projects/intellikeeper-hardware /Users/zeroisone/projects/intellikeeper-hardware/cmake-build-debug /Users/zeroisone/projects/intellikeeper-hardware/cmake-build-debug /Users/zeroisone/projects/intellikeeper-hardware/cmake-build-debug/CMakeFiles/manager.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/manager.dir/depend

