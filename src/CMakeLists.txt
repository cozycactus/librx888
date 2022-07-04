
add_library(rx888 SHARED librx888.c)
target_link_libraries(rx888 ${LIBUSB_LINK_LIBRARIES}) 

target_link_libraries(rx888 PkgConfig::LIBUSB)

target_include_directories(rx888 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>  # <prefix>/include
  ${LIBUSB_INCLUDE_DIRS}
  
  )

########################################################################
# Install built library files & utilities
########################################################################
install(TARGETS rx888 
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # .so/.dylib file
  )