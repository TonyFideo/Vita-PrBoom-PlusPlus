cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(taihen_stub_generator C)
include("${VITASDK}/share/vita.cmake" REQUIRED)

add_executable(taihen_exports exports.c)
target_include_directories(taihen_exports PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../taihen"
)

# Generate only the kernel-style import archive needed by SceShaccCgExt.
# This is a build-time import description, not a replacement for taiHEN on
# the console; the real taiHEN runtime remains an external prerequisite.
vita_create_stubs(taihen_stub taihen_exports
  "${CMAKE_CURRENT_SOURCE_DIR}/exports.yml"
  KERNEL
)
