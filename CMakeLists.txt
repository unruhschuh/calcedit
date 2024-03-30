cmake_minimum_required(VERSION 3.5)

set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE INTERNAL "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE INTERNAL "" FORCE)

set(FLTK_BUILD_TEST OFF)
set(FLTK_BUILD_EXAMPLES OFF)

add_subdirectory(deps/fltk)

project(calcedit)

set(SRC
  src/calcedit.cpp
  )

if (APPLE AND (NOT OPTION_APPLE_X11))
  add_executable(calcedit MACOSX_BUNDLE ${SRC})
else ()
  add_executable(calcedit WIN32 ${SRC})
endif()

if(WIN32)
  target_link_libraries(calcedit gdiplus)
endif()

if (APPLE)
  target_link_libraries(calcedit "-framework cocoa")
endif (APPLE)

set_property(TARGET calcedit PROPERTY CXX_STANDARD 17)
target_include_directories(calcedit PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/exprtk")
target_link_libraries(calcedit fltk)
