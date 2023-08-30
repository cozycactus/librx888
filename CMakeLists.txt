cmake_minimum_required(VERSION 3.0.0)

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

set(CMAKE_C_FLAGS "-std=c11 -pedantic-errors -Wall -Wextra -Werror")

include(GNUInstallDirs)
include(GenerateExportHeader)
include(CMakePackageConfigHelpers)

# Set the version information here
set(VERSION_INFO_MAJOR_VERSION 0) # increment major on api compatibility changes
set(VERSION_INFO_MINOR_VERSION 6) # increment minor on feature-level changes
set(VERSION_INFO_PATCH_VERSION git) # increment patch for bug fixes and docs

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBUSB libusb-1.0 IMPORTED_TARGET)
endif()

if(PKG_CONFIG_FOUND AND NOT LIBUSB_FOUND)
    message(FATAL_ERROR "LibUSB 1.0 required to compile rx888")
endif()

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

add_subdirectory(include)
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

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
