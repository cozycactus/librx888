# Set minimum required version for CMake
cmake_minimum_required(VERSION 3.10)

# Prevent in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds are not allowed. Please create a separate\
   build directory and run CMake from there.")
endif()

# workaround for https://gitlab.kitware.com/cmake/cmake/issues/16967
if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
    project(rx888)
else()
    project(rx888 C)
endif()

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

include(GNUInstallDirs)
include(GenerateExportHeader)
include(CMakePackageConfigHelpers)

# Set the version information here
set(VERSION_INFO_MAJOR_VERSION 1) # increment major on api compatibility changes
set(VERSION_INFO_MINOR_VERSION 0) # increment minor on feature-level changes
set(VERSION_INFO_PATCH_VERSION git) # increment patch for bug fixes and docs
include(Version) # setup version info

########################################################################
# Compiler specific setup
########################################################################
if(CMAKE_COMPILER_IS_GNUCC AND NOT WIN32)
    ADD_DEFINITIONS(-Wall)
    ADD_DEFINITIONS(-Wextra)
    ADD_DEFINITIONS(-Wno-unused-parameter)
    ADD_DEFINITIONS(-Wno-unused)
    ADD_DEFINITIONS(-Wsign-compare)
    ADD_DEFINITIONS(-Wdeclaration-after-statement)
    #http://gcc.gnu.org/wiki/Visibility
    add_definitions(-fvisibility=hidden)
elseif(MSVC14 OR MSVC14)
#pthread-w32 issue, timespec is now part of time.h
    ADD_DEFINITIONS(-D_TIMESPEC_DEFINED)
endif()

########################################################################
# Find build dependencies
########################################################################
find_package(Threads)
find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBUSB libusb-1.0 IMPORTED_TARGET)
    if(LIBUSB_LINK_LIBRARIES)
        set(LIBUSB_LIBRARIES "${LIBUSB_LINK_LIBRARIES}")
    endif()
else()
    set(LIBUSB_LIBRARIES "" CACHE STRING "manual libusb path")
    set(LIBUSB_INCLUDE_DIRS "" CACHE STRING "manual libusb includepath")
endif()

if(MSVC)
    set(THREADS_PTHREADS_LIBRARY "" CACHE STRING "manual pthread-win32 path")
    set(THREADS_PTHREADS_INCLUDE_DIR "" CACHE STRING "manual pthread-win32 includepath")
else()
    set(THREADS_PTHREADS_LIBRARY "" CACHE INTERNAL "manual pthread-win32 path")
    set(THREADS_PTHREADS_INCLUDE_DIR "" CACHE INTERNAL "manual pthread-win32 includepath")
endif()

if(PKG_CONFIG_FOUND AND NOT LIBUSB_FOUND)
    message(FATAL_ERROR "LibUSB 1.0 required to compile rx888")
endif()
if(NOT THREADS_FOUND)
    message(FATAL_ERROR "pthreads(-win32) required to compile rx888")
endif()

########################################################################
# Create uninstall target
########################################################################
configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)

add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

########################################################################
# Install public header files
########################################################################
install(FILES
    include/librx888.h
    
    DESTINATION include
)

########################################################################
# Add subdirectories
########################################################################
add_subdirectory(src)

########################################################################
# Create Pkg Config File
########################################################################
FOREACH(inc ${LIBUSB_INCLUDEDIR})
    LIST(APPEND RX888_PC_CFLAGS "-I${inc}")
ENDFOREACH(inc)

FOREACH(lib ${LIBUSB_LIBRARY_DIRS})
    LIST(APPEND RX888_PC_LIBS "-L${lib}")
ENDFOREACH(lib)

# use space-separation format for the pc file
STRING(REPLACE ";" " " RX888_PC_CFLAGS "${RX888_PC_CFLAGS}")
STRING(REPLACE ";" " " RX888_PC_LIBS "${RX888_PC_LIBS}")

# unset these vars to avoid hard-coded paths to cross environment
IF(CMAKE_CROSSCOMPILING)
    UNSET(RX888_PC_CFLAGS)
    UNSET(RX888_PC_LIBS)
ENDIF(CMAKE_CROSSCOMPILING)

set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix \${prefix})
set(includedir \${prefix}/include)
set(libdir \${exec_prefix}/lib)

CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/librx888.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/librx888.pc
@ONLY)

INSTALL(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/librx888.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

########################################################################
# Create CMake Config File
########################################################################
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/rx888/rx888ConfigVersion.cmake"
  VERSION ${VERSION}
  COMPATIBILITY AnyNewerVersion
  )

configure_file(cmake/rx888Config.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/rx888/rx888Config.cmake"
  COPYONLY
  )

set(ConfigPackageLocation lib/cmake/rx888)
install(EXPORT RX888-export
  FILE rx888Targets.cmake
  NAMESPACE rx888::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rx888/
  )
install(
  FILES
  cmake/rx888Config.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/rx888/rx888ConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rx888/
  COMPONENT Devel
  )

########################################################################
# Print Summary
########################################################################
MESSAGE(STATUS "Building for version: ${VERSION} / ${LIBVER}")
MESSAGE(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
