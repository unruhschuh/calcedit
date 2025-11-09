cmake_minimum_required(VERSION 3.5)

set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE INTERNAL "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE INTERNAL "" FORCE)

set(FLTK_BUILD_TEST OFF CACHE INTERNAL "" FORCE)
set(FLTK_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(FLTK_BUILD_FLUID OFF CACHE INTERNAL "" FORCE)

add_subdirectory(deps/fltk)
add_subdirectory(deps/fmt)
add_subdirectory(calceditlib)

project(calcedit)

set(SRC
  src/calcedit.cpp
  )

if (APPLE AND (NOT OPTION_APPLE_X11))
  add_executable(calcedit MACOSX_BUNDLE ${SRC})
else ()
  add_executable(calcedit WIN32 ${SRC})
endif()

if (MSVC)
  target_compile_options(calcedit PRIVATE /bigobj)
endif()

if(WIN32)
  target_link_libraries(calcedit gdiplus)
endif()

if (APPLE)
  target_link_libraries(calcedit "-framework cocoa")
endif (APPLE)

set_property(TARGET calcedit PROPERTY CXX_STANDARD 17)
target_include_directories(calcedit PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/deps/exprtk")
target_link_libraries(calcedit fltk fmt calceditlib)

