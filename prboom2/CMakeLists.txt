cmake_minimum_required(VERSION 3.0)

project("Vita-PrBoom-PlusPlus"
    VERSION 2.6.66
    HOMEPAGE_URL "https://github.com/coelckers/prboom-plus")

# The PrBoom+ sources use C99-era identifiers such as false/true as enum
# members; keep the project on the language level it was written for.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Keep the upstream release build default while retaining the Vita-specific
# compiler flags below.
set(default_build_type "RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(TargetArch)

include(TestBigEndian)
TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

include(GNUInstallDirs)

# Automated dependencies discovery, mostly needed for MSVC
target_architecture(TARGET_ARCH)
if(${TARGET_ARCH} MATCHES "i386")
    set(DEPENDENCY_SUFFIX x86)
elseif(${TARGET_ARCH} MATCHES "x86_64")
    set(DEPENDENCY_SUFFIX x64)
endif()
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../dependencies_${DEPENDENCY_SUFFIX}")

set(PACKAGE_NAME "${PROJECT_NAME}")
set(PACKAGE_TARNAME "prboom-plus")
# Keep the numeric package version for compatibility parsing, while exposing
# the Vita release identifier in user-facing strings.
set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(PACKAGE_DISPLAY_VERSION "2.6.66-1.0")
set(PACKAGE_HOMEPAGE "${PROJECT_HOMEPAGE_URL}")
set(PACKAGE_STRING "${PROJECT_NAME} ${PACKAGE_DISPLAY_VERSION}")

# Vita things
if(VITA)
    include("${VITASDK}/share/vita.cmake")
    set(VITA_TITLEID "PBR002666")
    set(VITA_NAME "Vita-PrBoom++")
    # VitaSDK requires the Vita version field to use the ##.## format.
    set(VITA_VERSION "01.00")
endif()

include(CheckSymbolExists)

check_symbol_exists(stricmp "string.h" HAVE_STRICMP)
if(NOT HAVE_STRICMP)
    add_definitions("-Dstricmp=strcasecmp")
endif()
check_symbol_exists(strnicmp "string.h" HAVE_STRNICMP)
if(NOT HAVE_STRNICMP)
    add_definitions("-Dstrnicmp=strncasecmp")
endif()

check_symbol_exists(getopt "unistd.h" HAVE_GETOPT)
check_symbol_exists(mmap "sys/mman.h" HAVE_MMAP)
check_symbol_exists(CreateFileMapping "windows.h" HAVE_CREATE_FILE_MAPPING)
if(NOT WIN32)
    set(CMAKE_REQUIRED_DEFINITIONS_PREV ${CMAKE_REQUIRED_DEFINITIONS})
    set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -D_GNU_SOURCE)
    check_symbol_exists(sched_setaffinity "sched.h" HAVE_SCHED_SETAFFINITY)
    set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS_PREV})
    if(HAVE_SCHED_SETAFFINITY)
        add_definitions(-D_GNU_SOURCE)
    endif()
endif()
check_symbol_exists(usleep "unistd.h" HAVE_USLEEP)
check_symbol_exists(strsignal "string.h" HAVE_STRSIGNAL)
check_symbol_exists(mkstemp "stdlib.h" HAVE_MKSTEMP)

include(CheckIncludeFile)

check_include_file("sys/wait.h" HAVE_SYS_WAIT_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("asm/byteorder.h" HAVE_ASM_BYTEORDER_H)
check_include_file("dirent.h" HAVE_DIRENT_H)

option(BUILD_GL "Enable OpenGL rendering code" ON)
if(BUILD_GL)
    if(VITA)
        # we got vitaGL
        set(OPENGL_FOUND TRUE)
        set(OPENGL_GLU_FOUND TRUE)
    else()
        set(OpenGL_GL_PREFERENCE LEGACY)
        find_package(OpenGL)
    endif()
endif()

find_package(SDL2 2.0.7 REQUIRED)

option(WITH_IMAGE "Use SDL2_image if available" ON)
if(WITH_IMAGE)
    find_package(SDL2_image)
    if(SDL2_IMAGE_FOUND)
        set(HAVE_LIBSDL2_IMAGE TRUE)
    endif()
endif()

option(WITH_MIXER "Use SDL2_mixer if available" ON)
if(WITH_MIXER)
    find_package(SDL2_mixer)
    if(SDL2_MIXER_FOUND)
        set(HAVE_LIBSDL2_MIXER TRUE)
    endif()
endif()

option(WITH_NET "Use SDL2_net if available" ON)
if(WITH_NET)
    find_package(SDL2_net)
    if(SDL2_NET_FOUND)
        set(HAVE_NET TRUE)
        set(USE_SDL_NET TRUE)
    endif()
endif()

option(WITH_PCRE "Use PCRE if available" ON)
if(WITH_PCRE)
    find_package(PCREPOSIX)
    if(PCREPOSIX_FOUND)
        set(HAVE_LIBPCREPOSIX TRUE)
    endif()
endif()

option(WITH_ZLIB "Use ZLIB if available" ON)
if(WITH_ZLIB)
    find_package(ZLIB)
    if(ZLIB_FOUND)
        set(HAVE_LIBZ TRUE)
    endif()
endif()

option(WITH_MAD "Use libmad if available" ON)
if(WITH_MAD)
    find_package(LibMad)
    if(LIBMAD_FOUND)
        set(HAVE_LIBMAD TRUE)
    endif()
endif()

# VitaSDK provides mpg123, while it does not provide libmad.  Keep the
# existing libmad backend for desktop builds and use a dedicated mpg123
# backend for Vita instead of pretending the two APIs are interchangeable.
option(WITH_MPG123 "Use mpg123 for the Vita MP3 player if available" ON)
if(WITH_MPG123 AND VITA)
    find_package(Mpg123)
    if(MPG123_FOUND)
        set(HAVE_LIBMPG123 TRUE)
        message(STATUS "Vita mpg123 MP3 player: enabled")
    else()
        message(STATUS "Vita mpg123 MP3 player: unavailable")
    endif()
endif()

option(WITH_FLUIDSYNTH "Use FluidSynth if available" ON)
if(WITH_FLUIDSYNTH)
    find_package(FluidSynth)
    if(FLUIDSYNTH_FOUND)
        set(HAVE_LIBFLUIDSYNTH TRUE)
        message(STATUS "FluidSynth MIDI player: enabled")
    else()
        message(STATUS "FluidSynth MIDI player: unavailable")
    endif()
endif()

option(WITH_DUMB "Use DUMB if available" ON)
if(WITH_DUMB)
    if(VITA)
        # The VitaSDK does not ship libdumb.  Build the vendored DUMB source
        # as a static target instead of treating the feature as unavailable.
        set(DUMB_FOUND TRUE)
        set(HAVE_LIBDUMB TRUE)
        set(DUMB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/dumb/include")
        message(STATUS "DUMB tracker player: enabled (vendored Vita build)")
    else()
        find_package(DUMB)
        if(DUMB_FOUND)
            set(HAVE_LIBDUMB TRUE)
        endif()
    endif()
endif()

option(WITH_VORBISFILE "Use vorbisfile if available" ON)
if(WITH_VORBISFILE)
    find_package(Vorbis)
    if(VORBIS_FOUND)
        set(HAVE_LIBVORBISFILE TRUE)
        message(STATUS "Vorbis/Ogg music player: enabled")
    else()
        message(STATUS "Vorbis/Ogg music player: unavailable")
    endif()
endif()

option(WITH_PORTMIDI "Use PortMidi if available" ON)
if(WITH_PORTMIDI)
    find_package(PortMidi)
    if(PortMidi_FOUND)
        set(HAVE_LIBPORTMIDI TRUE)
    endif()
endif()

option(WITH_ALSA "Use ALSA MIDI if available" ON)
if(WITH_ALSA AND NOT VITA)
    find_package(ALSA)
    if(ALSA_FOUND)
        set(HAVE_ALSA TRUE)
    endif()
endif()

# Keep the temporary Vita diagnostic instrumentation enabled while the modern
# VitaGL migration is being validated. It can be disabled for a release build
# with -DVITA_PRBOOM_DIAGNOSTICS=OFF.
option(VITA_PRBOOM_DIAGNOSTICS "Log Vita GL scene and WAD rendering diagnostics" ON)

# Keep the profiling instrumentation and its launcher control in sync.  When
# disabled, VitaGL is built without HAVE_PROFILING and the launcher omits the
# Profiling option instead of exposing a setting that cannot do anything.
option(VITA_GL_PROFILING "Build VitaGL profiling instrumentation and expose its launcher option" ON)

if(VITA)
    # Use the modern VitaGL source bundled with this Vita build.
    set(VITAGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vita/vitaGL-modern)
    set(VITASHARK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vita/vitaShaRK)
    set(VITAGL_LIBRARY ${VITAGL_DIR}/libvitaGL.a)
    set(VITASHARK_LIBRARY ${VITASHARK_DIR}/libvitashark.a)

    # vitaShaRK is built locally so this test tree does not install anything
    # into the user's global VitaSDK.  Its header also supplies the dependency
    # required by the modern VitaGL header.
    add_custom_target(build_vitaShaRK ALL
                   COMMAND make
                   WORKING_DIRECTORY ${VITASHARK_DIR}
                   BYPRODUCTS ${VITASHARK_LIBRARY}
                   COMMENT "Building local vitaShaRK")
    add_library(vitaShaRK STATIC IMPORTED GLOBAL)
    set_target_properties(vitaShaRK PROPERTIES
        IMPORTED_LOCATION "${VITASHARK_LIBRARY}")
    add_dependencies(vitaShaRK build_vitaShaRK)

    # ENABLE_LEGACY_PIPELINE is required by PrBoom+'s client-array bridge.
    # Keep compiler diagnostics enabled while this migration is being tested.
    # Vita3K does not support the modern VitaGL splashscreen's secondary GXM
    # context. Disable it for this migration build; the main rendering
    # context remains unchanged.
    set(VITA_GL_DIAGNOSTICS_ARG)
    if(VITA_PRBOOM_DIAGNOSTICS)
        set(VITA_GL_DIAGNOSTICS_ARG VITA_GL_DIAGNOSTICS=1)
    endif()
    set(VITA_GL_PROFILING_ARG)
    if(VITA_GL_PROFILING)
        set(VITA_GL_PROFILING_ARG HAVE_PROFILING=1)
    endif()
    add_custom_target(build_vitaGL ALL
                   COMMAND make ENABLE_LEGACY_PIPELINE=1 HAVE_SHARK_LOG=1 LOG_ERRORS=1 ${VITA_GL_PROFILING_ARG} ${VITA_GL_DIAGNOSTICS_ARG} NO_SPLASHSCREEN=1 NO_TILE_CLIPPER=1
                   WORKING_DIRECTORY ${VITAGL_DIR}
                   BYPRODUCTS ${VITAGL_LIBRARY}
                   DEPENDS build_vitaShaRK
                   COMMENT "Building modern VitaGL")
    add_library(vitaGL STATIC IMPORTED GLOBAL)
    set_target_properties(vitaGL PROPERTIES
        IMPORTED_LOCATION "${VITAGL_LIBRARY}")
    add_dependencies(vitaGL build_vitaGL)

    # SceShaccCgExt and taihen_stub are required by vitaShaRK at final link
    # time.  Prefer explicit local paths; otherwise fall back to libraries
    # installed in VitaSDK.  This keeps the dependency visible instead of
    # silently switching back to the old VitaGL backend.
    set(VITA_SCE_SHACCCG_EXT_LIBRARY "" CACHE FILEPATH
        "Path to libSceShaccCgExt.a (or an installed SceShaccCgExt library)")
    set(VITA_TAIHEN_STUB_LIBRARY "" CACHE FILEPATH
        "Path to libtaihen_stub.a (or an installed taihen_stub library)")
    if(NOT VITA_SCE_SHACCCG_EXT_LIBRARY)
        find_library(VITA_SCE_SHACCCG_EXT_LIBRARY
            NAMES SceShaccCgExt
            PATHS ${VITASDK}/arm-vita-eabi/lib
            NO_DEFAULT_PATH)
    endif()
    if(NOT VITA_TAIHEN_STUB_LIBRARY)
        find_library(VITA_TAIHEN_STUB_LIBRARY
            NAMES taihen_stub
            PATHS ${VITASDK}/arm-vita-eabi/lib
            NO_DEFAULT_PATH)
    endif()
    if(NOT VITA_SCE_SHACCCG_EXT_LIBRARY)
        set(VITA_SCE_SHACCCG_EXT_LIBRARY SceShaccCgExt)
        message(STATUS "SceShaccCgExt not found locally; final link expects an installed SceShaccCgExt library")
    endif()
    if(NOT VITA_TAIHEN_STUB_LIBRARY)
        set(VITA_TAIHEN_STUB_LIBRARY taihen_stub)
        message(STATUS "taihen_stub not found locally; final link expects an installed taihen_stub library")
    endif()
endif()

set(CMAKE_REQUIRED_INCLUDES_PREV ${CMAKE_REQUIRED_INCLUDES})
set(CMAKE_REQUIRED_LIBRARIES_PREV ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${SDL2_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${SDL2_LIBRARIES})
if(VITA)
    # the check fails for our SDL.h for some reason
    set(HAVE_SDL_JOYSTICKGETAXIS TRUE)
else()
    check_symbol_exists(SDL_JoystickGetAxis "SDL.h" HAVE_SDL_JOYSTICKGETAXIS)
endif()
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_PREV})
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_PREV})

set(DOOMWADDIR "${CMAKE_INSTALL_PREFIX}/share/games/doom" CACHE PATH "Path to look for WAD files")
set(PRBOOMDATADIR "${CMAKE_INSTALL_PREFIX}/share/${PACKAGE_TARNAME}" CACHE PATH "Path to install supplemental files")

option(SIMPLECHECKS "Enable checks which only impose significant overhead if a posible error is detected" ON)
option(ZONEIDCHECK "Enable id checks on zone blocks, to detect corrupted and illegally freed blocks" ON)

# Debug options, disabled by default
option(RANGECHECK "Enable internal range checking" OFF)
option(INSTRUMENTED "Enable real-time memory allocation statistics, and extra debugging features" OFF)
option(TIMEDIAG "Enable creation of time stamps each time a lump is locked, and reporting of lumps locked for long periods of time" OFF)
option(HEAPCHECK "Turn on continuous heap checking (very slow)" OFF)
option(HEAPDUMP "Turn on dumping the heap state for debugging" OFF)

configure_file(cmake/config.h.cin config.h)

add_definitions(-DHAVE_CONFIG_H)

if (MSVC)
    add_definitions("/D_CRT_SECURE_NO_WARNINGS")
else()
    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} \
        -Wall -Wno-missing-field-initializers -Wwrite-strings -Wundef \
        -Wbad-function-cast -Wcast-align -Wcast-qual -Wdeclaration-after-statement \
        -Wpointer-arith -Wno-unused -Wno-switch -Wno-sign-compare -Wno-pointer-sign \
        -ffast-math"
    )
    if (VITA)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtune=cortex-a9 -mfpu=neon -mthumb -O3 -g -fsigned-char -fno-short-enums")
        set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g")
    endif()
endif()

set(PRBOOM_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(WAD_DATA prboom-plus.wad)
set(WAD_DATA_PATH "${PRBOOM_OUTPUT_PATH}/${WAD_DATA}")

if(NOT CMAKE_CROSSCOMPILING OR VITA)
    add_subdirectory(data)
    add_subdirectory(doc)
endif()
if(VITA)
    add_subdirectory(vita/libtess)
    add_subdirectory(vita/launcher)
    if(WITH_DUMB)
        add_subdirectory(thirdparty/dumb EXCLUDE_FROM_ALL)
    endif()
endif()
add_subdirectory(src)
