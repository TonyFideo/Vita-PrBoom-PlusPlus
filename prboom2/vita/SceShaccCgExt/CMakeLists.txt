cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(SceShaccCgExt)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -ffast-math -mtune=cortex-a9 -mfpu=neon -fno-optimize-sibling-calls -ftree-vectorize -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -mtune=cortex-a9 -mfpu=neon -fno-optimize-sibling-calls -ftree-vectorize -Wall -fno-exceptions -fno-rtti -std=gnu++11 -fpermissive")

add_library(${PROJECT_NAME}
  include/shacccg_ext.h
  include/sce_intrinsics.h
  src/shacccg_ext.c
  src/shacccg_ext_oop.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  include
  "${CMAKE_CURRENT_SOURCE_DIR}/../taihen")

set(TAIHEN_STUB_LIBRARY "" CACHE FILEPATH
  "Local taiHEN import archive used to link SceShaccCgExt")
if(TAIHEN_STUB_LIBRARY)
  set(TAIHEN_STUB_LINK ${TAIHEN_STUB_LIBRARY})
else()
  set(TAIHEN_STUB_LINK taihen_stub)
endif()

target_link_libraries(${PROJECT_NAME}
  ${TAIHEN_STUB_LINK}
  SceLibKernel_stub
  SceKernelModulemgr_stub
)

install(TARGETS ${PROJECT_NAME})

install(FILES include/shacccg_ext.h
  DESTINATION include
)
