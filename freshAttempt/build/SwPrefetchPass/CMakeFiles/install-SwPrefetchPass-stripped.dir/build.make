# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /n/eecs583a/home/jackholl/EECS583-reproduce-cgo2017-paper/freshAttempt

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /n/eecs583a/home/jackholl/EECS583-reproduce-cgo2017-paper/freshAttempt/build

# Utility rule file for install-SwPrefetchPass-stripped.

# Include any custom commands dependencies for this target.
include SwPrefetchPass/CMakeFiles/install-SwPrefetchPass-stripped.dir/compiler_depend.make

# Include the progress variables for this target.
include SwPrefetchPass/CMakeFiles/install-SwPrefetchPass-stripped.dir/progress.make

SwPrefetchPass/CMakeFiles/install-SwPrefetchPass-stripped:
	cd /n/eecs583a/home/jackholl/EECS583-reproduce-cgo2017-paper/freshAttempt/build/SwPrefetchPass && /usr/bin/cmake -DCMAKE_INSTALL_COMPONENT="SwPrefetchPass" -DCMAKE_INSTALL_DO_STRIP=1 -P /n/eecs583a/home/jackholl/EECS583-reproduce-cgo2017-paper/freshAttempt/build/cmake_install.cmake

install-SwPrefetchPass-stripped: SwPrefetchPass/CMakeFiles/install-SwPrefetchPass-stripped
install-SwPrefetchPass-stripped: SwPrefetchPass/CMakeFiles/install-SwPrefetchPass-stripped.dir/build.make
.PHONY : install-SwPrefetchPass-stripped

# Rule to build all files generated by this target.
SwPrefetchPass/CMakeFiles/install-SwPrefetchPass-stripped.dir/build: install-SwPrefetchPass-stripped
.PHONY : SwPrefetchPass/CMakeFiles/install-SwPrefetchPass-stripped.dir/build

SwPrefetchPass/CMakeFiles/install-SwPrefetchPass-stripped.dir/clean:
	cd /n/eecs583a/home/jackholl/EECS583-reproduce-cgo2017-paper/freshAttempt/build/SwPrefetchPass && $(CMAKE_COMMAND) -P CMakeFiles/install-SwPrefetchPass-stripped.dir/cmake_clean.cmake
.PHONY : SwPrefetchPass/CMakeFiles/install-SwPrefetchPass-stripped.dir/clean

SwPrefetchPass/CMakeFiles/install-SwPrefetchPass-stripped.dir/depend:
	cd /n/eecs583a/home/jackholl/EECS583-reproduce-cgo2017-paper/freshAttempt/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /n/eecs583a/home/jackholl/EECS583-reproduce-cgo2017-paper/freshAttempt /n/eecs583a/home/jackholl/EECS583-reproduce-cgo2017-paper/freshAttempt/SwPrefetchPass /n/eecs583a/home/jackholl/EECS583-reproduce-cgo2017-paper/freshAttempt/build /n/eecs583a/home/jackholl/EECS583-reproduce-cgo2017-paper/freshAttempt/build/SwPrefetchPass /n/eecs583a/home/jackholl/EECS583-reproduce-cgo2017-paper/freshAttempt/build/SwPrefetchPass/CMakeFiles/install-SwPrefetchPass-stripped.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : SwPrefetchPass/CMakeFiles/install-SwPrefetchPass-stripped.dir/depend

