
add_library(rx888 SHARED librx888.c)
target_compile_options(rx888 PRIVATE -fPIC)

target_link_libraries(rx888 ${LIBUSB_LINK_LIBRARIES}) 

target_link_libraries(rx888 PkgConfig::LIBUSB)

target_include_directories(rx888 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>  # <prefix>/include
  ${LIBUSB_INCLUDE_DIRS}
  
  )

########################################################################
# Build utility
########################################################################
add_executable(rx888_test rx888_test.c)
add_executable(rx888_rec rx888_rec.c)
set(INSTALL_TARGETS rx888_test rx888)

target_link_libraries(rx888_test rx888
    ${LIBUSB_LINK_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
target_link_libraries(rx888_rec rx888
    ${LIBUSB_LINK_LIBRARIES}
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