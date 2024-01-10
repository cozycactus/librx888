########################################################################
# Setup shared library variant
########################################################################
add_library(rx888 SHARED librx888.c rx888_variant.c bbrf103_variant.c
                          hf103_variant.c rx888mk2_variant.c)

#add_library(librx888::rx888 ALIAS rx888)

#target_compile_options(rx888 PRIVATE -fPIC)

target_link_libraries(rx888 ${LIBUSB_LIBRARIES} ${THREADS_PTHREADS_LIBRARY}) 

#target_link_libraries(rx888 PkgConfig::LIBUSB)

target_include_directories(rx888 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>  # <prefix>/include
  ${LIBUSB_INCLUDE_DIRS}
  ${THREADS_PTHREADS_INCLUDE_DIR}
  )
  set_target_properties(rx888 PROPERTIES DEFINE_SYMBOL "rx888_EXPORTS")
  set_target_properties(rx888 PROPERTIES OUTPUT_NAME rx888)
  set_target_properties(rx888 PROPERTIES SOVERSION ${MAJOR_VERSION})
  set_target_properties(rx888 PROPERTIES VERSION ${LIBVER})
  generate_export_header(rx888)

########################################################################
# Setup static library variant
########################################################################
add_library(rx888_static STATIC librx888.c rx888_variant.c bbrf103_variant.c
                          hf103_variant.c rx888mk2_variant.c)
target_link_libraries(rx888 ${LIBUSB_LIBRARIES} ${THREADS_PTHREADS_LIBRARY})
target_include_directories(rx888_static PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>  # <prefix>/include
  ${LIBUSB_INCLUDE_DIRS}
  ${THREADS_PTHREADS_INCLUDE_DIR}
  )
set_property(TARGET rx888_static APPEND PROPERTY COMPILE_DEFINITIONS "rx888_STATIC" )
if(NOT WIN32)
# Force same library filename for static and shared variants of the library
set_target_properties(rx888_static PROPERTIES OUTPUT_NAME rx888)
endif()
generate_export_header(rx888_static)



########################################################################
# Setup libraries used in executables
########################################################################
add_library(convenience_static STATIC
    convenience/convenience.c
)
target_include_directories(convenience_static
  PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(WIN32)
add_library(libgetopt_static STATIC
    getopt/getopt.c
)
target_link_libraries(convenience_static
    rx888
)
endif()

########################################################################
# Build utility
########################################################################
add_executable(rx888_test rx888_test.c)
add_executable(rx888_rec rx888_rec.c)
set(INSTALL_TARGETS rx888_test rx888_rec rx888_static)

target_link_libraries(rx888_test rx888_static convenience_static
    ${LIBUSB_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
target_link_libraries(rx888_rec rx888_static convenience_static
    ${LIBUSB_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

if(UNIX)
if(APPLE OR CMAKE_SYSTEM MATCHES "OpenBSD")
    target_link_libraries(rx888_test m)
else()
    target_link_libraries(rx888_test m rt)
endif()
endif()

########################################################################
# Install built library files & utilities
########################################################################
install(TARGETS rx888 
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # .so/.dylib file
  )
install(TARGETS rx888_test rx888_rec
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  )