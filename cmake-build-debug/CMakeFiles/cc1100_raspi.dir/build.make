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
include CMakeFiles/cc1100_raspi.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/cc1100_raspi.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/cc1100_raspi.dir/flags.make

CMakeFiles/cc1100_raspi.dir/network/cc1100_raspi.o: CMakeFiles/cc1100_raspi.dir/flags.make
CMakeFiles/cc1100_raspi.dir/network/cc1100_raspi.o: ../network/cc1100_raspi.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zeroisone/projects/intellikeeper-hardware/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/cc1100_raspi.dir/network/cc1100_raspi.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/cc1100_raspi.dir/network/cc1100_raspi.o -c /Users/zeroisone/projects/intellikeeper-hardware/network/cc1100_raspi.cpp

CMakeFiles/cc1100_raspi.dir/network/cc1100_raspi.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cc1100_raspi.dir/network/cc1100_raspi.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/zeroisone/projects/intellikeeper-hardware/network/cc1100_raspi.cpp > CMakeFiles/cc1100_raspi.dir/network/cc1100_raspi.i

CMakeFiles/cc1100_raspi.dir/network/cc1100_raspi.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cc1100_raspi.dir/network/cc1100_raspi.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/zeroisone/projects/intellikeeper-hardware/network/cc1100_raspi.cpp -o CMakeFiles/cc1100_raspi.dir/network/cc1100_raspi.s

# Object files for target cc1100_raspi
cc1100_raspi_OBJECTS = \
"CMakeFiles/cc1100_raspi.dir/network/cc1100_raspi.o"

# External object files for target cc1100_raspi
cc1100_raspi_EXTERNAL_OBJECTS =

libcc1100_raspi.a: CMakeFiles/cc1100_raspi.dir/network/cc1100_raspi.o
libcc1100_raspi.a: CMakeFiles/cc1100_raspi.dir/build.make
libcc1100_raspi.a: CMakeFiles/cc1100_raspi.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/zeroisone/projects/intellikeeper-hardware/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libcc1100_raspi.a"
	$(CMAKE_COMMAND) -P CMakeFiles/cc1100_raspi.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/cc1100_raspi.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/cc1100_raspi.dir/build: libcc1100_raspi.a

.PHONY : CMakeFiles/cc1100_raspi.dir/build

CMakeFiles/cc1100_raspi.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/cc1100_raspi.dir/cmake_clean.cmake
.PHONY : CMakeFiles/cc1100_raspi.dir/clean

CMakeFiles/cc1100_raspi.dir/depend:
	cd /Users/zeroisone/projects/intellikeeper-hardware/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/zeroisone/projects/intellikeeper-hardware /Users/zeroisone/projects/intellikeeper-hardware /Users/zeroisone/projects/intellikeeper-hardware/cmake-build-debug /Users/zeroisone/projects/intellikeeper-hardware/cmake-build-debug /Users/zeroisone/projects/intellikeeper-hardware/cmake-build-debug/CMakeFiles/cc1100_raspi.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/cc1100_raspi.dir/depend
