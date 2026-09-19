# DUMB 1.0.0, vendored for the Vita build.
#
# This is deliberately kept separate from the ZMusic build file.  That file
# enables x86 SSE and calls ZMusic-specific helper functions which do not
# exist in PrBoom++.

file(GLOB_RECURSE PRBOOM_DUMB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(prboom_dumb STATIC ${PRBOOM_DUMB_SOURCES})

target_include_directories(prboom_dumb
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# VitaSDK provides alloca.h, while its libc does not provide the non-standard
# itoa() used by one of DUMB's legacy readers.
target_compile_definitions(prboom_dumb
    PRIVATE
        HAVE_ALLOCA_H=1
        NEED_ITOA=1
)

if(VORBIS_FOUND)
    target_compile_definitions(prboom_dumb PRIVATE PRBOOM_DUMB_HAVE_VORBIS=1)
    target_include_directories(prboom_dumb PRIVATE ${VORBIS_INCLUDE_DIRS})
    target_link_libraries(prboom_dumb PUBLIC ${VORBIS_LIBRARIES})
endif()

# DUMB contains optional x86 SSE paths.  No _USE_SSE or -msse is added here;
# Vita uses the portable scalar implementation on ARM.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(prboom_dumb PRIVATE
        -Wno-pointer-sign
        -Wno-uninitialized
        -Wno-unused-but-set-variable
    )
endif()

set_target_properties(prboom_dumb PROPERTIES
    POSITION_INDEPENDENT_CODE OFF
)
