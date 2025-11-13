cmake_minimum_required(VERSION 3.5)

project(calceditlib)

set(SRC
  src/calceditlib.cpp
  src/exprtk_complex_adaptor.hpp
  include/calceditlib.h
  include/complex_type.hpp
  )

add_library(calceditlib ${SRC})

set_property(TARGET calceditlib PROPERTY CXX_STANDARD 17)
target_include_directories(calceditlib PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/../deps/exprtk" PUBLIC include)
target_link_libraries(calceditlib fmt)

