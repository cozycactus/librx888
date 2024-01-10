include(FindPkgConfig)
pkg_check_modules(LIBUSB libusb-1.0 IMPORTED_TARGET)

get_filename_component(RX888_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

if(NOT TARGET rx888::rx888)
  include("${RTLSDR_CMAKE_DIR}/rx888Targets.cmake")
endif()
