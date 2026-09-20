# PrBoom-Plus WAD creation tool

if(NOT CMAKE_CROSSCOMPILING)
    set(RDATAWAD_SOURCES
        rd_main.c
        rd_util.c
        rd_util.h
        rd_output.c
        rd_output.h
        rd_sound.c
        rd_sound.h
        rd_palette.c
        rd_palette.h
        rd_graphic.c
        rd_graphic.h
    )

    add_executable(rdatawad ${RDATAWAD_SOURCES})
    target_include_directories(rdatawad PRIVATE ${CMAKE_BINARY_DIR})

    set(RDATAWAD_COMMAND rdatawad)
    set(RDATAWAD_DEPENDS rdatawad)
elseif(VITA)
    # The Vita toolchain cannot execute a target binary while building the
    # internal WAD. Build the small WAD generator with a host compiler instead.
    set(VITA_HOST_CC "" CACHE FILEPATH "Host C compiler used to build rdatawad")
    if(NOT VITA_HOST_CC)
        find_program(VITA_HOST_CC
            NAMES gcc cc clang
            HINTS
                "$ENV{MSYS2_ROOT}/ucrt64/bin"
                "$ENV{MSYS2_ROOT}/mingw64/bin"
                "C:/msys64/ucrt64/bin"
                "C:/msys64/mingw64/bin"
                "C:/devkitPro/msys2/ucrt64/bin"
                "C:/devkitPro/msys2/mingw64/bin"
        )
    endif()
    if(NOT VITA_HOST_CC)
        message(FATAL_ERROR
            "A host C compiler is required to generate prboom-plus.wad. "
            "Run CMake from MSYS2 or set VITA_HOST_CC to gcc/clang."
        )
    endif()

    get_filename_component(VITA_HOST_CC_DIR "${VITA_HOST_CC}" DIRECTORY)

    set(RDATAWAD_HOST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/rd_main.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/rd_util.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/rd_output.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/rd_sound.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/rd_palette.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/rd_graphic.c"
    )
    # Keep the output name different from the custom target name. This also
    # works from MSYS2, where host executables do not necessarily use .exe in
    # their CMake-side path.
    set(RDATAWAD_HOST "${CMAKE_CURRENT_BINARY_DIR}/rdatawad-host-tool")

    add_custom_command(
        OUTPUT "${RDATAWAD_HOST}"
        COMMAND "${CMAKE_COMMAND}" -E env
            "PATH=${VITA_HOST_CC_DIR}:/usr/bin"
            "${VITA_HOST_CC}"
            -std=c99
            "-I${CMAKE_BINARY_DIR}"
            "-I${CMAKE_CURRENT_SOURCE_DIR}"
            ${RDATAWAD_HOST_SOURCES}
            -o "${RDATAWAD_HOST}"
        DEPENDS ${RDATAWAD_HOST_SOURCES} "${CMAKE_BINARY_DIR}/config.h"
        COMMENT "Building host WAD generator"
        VERBATIM
    )
    add_custom_target(rdatawad-host DEPENDS "${RDATAWAD_HOST}")

    set(RDATAWAD_COMMAND "${RDATAWAD_HOST}")
    set(RDATAWAD_DEPENDS rdatawad-host)
endif()


# PrBoom-Plus internal WAD

set(PALETTE
    palette.rgb
)

set(LUMPS
    lumps/switches.lmp
    lumps/animated.lmp
    lumps/crbrick.lmp
    lumps/crtan.lmp
    lumps/crgray.lmp
    lumps/crgreen.lmp
    lumps/crbrown.lmp
    lumps/crgold.lmp
    lumps/crred.lmp
    lumps/crblue.lmp
    lumps/crblue2.lmp
    lumps/crorange.lmp
    lumps/cryellow.lmp
    lumps/crblack.lmp
    lumps/crpurple.lmp
    lumps/crwhite.lmp
)

set(COLORMAPS 
    lumps/watermap.lmp
)

set(TABLES
    lumps/sinetabl.lmp
    lumps/tangtabl.lmp
    lumps/tantoang.lmp
    lumps/gammatbl.lmp
    lumps/chexdeh.lmp
    lumps/bfgbex.lmp
    lumps/nervebex.lmp
    lumps/glshadow.lmp
    lumps/glfp.lmp
    lumps/glvp.lmp
    lumps/-prbhud-.lmp
    lumps/m_ammo.lmp
    lumps/m_armour.lmp
    lumps/m_arrow.lmp
    lumps/m_health.lmp
    lumps/m_key.lmp
    lumps/m_normal.lmp
    lumps/m_shadow.lmp
    lumps/m_power.lmp
    lumps/m_weap.lmp
    lumps/m_mark.lmp
)

set(SOUNDS
    sounds/dsdgsit.wav
    sounds/dsdgatk.wav
    sounds/dsdgact.wav
    sounds/dsdgdth.wav
    sounds/dsdgpain.wav
)

set(GRAPHICS
    graphics/dig0.ppm
    graphics/dig1.ppm
    graphics/dig2.ppm
    graphics/dig3.ppm
    graphics/dig4.ppm
    graphics/dig5.ppm
    graphics/dig6.ppm
    graphics/dig7.ppm
    graphics/dig8.ppm
    graphics/dig9.ppm
    graphics/diga.ppm
    graphics/digb.ppm
    graphics/digc.ppm
    graphics/digd.ppm
    graphics/dige.ppm
    graphics/digf.ppm
    graphics/digg.ppm
    graphics/digh.ppm
    graphics/digi.ppm
    graphics/digj.ppm
    graphics/digk.ppm
    graphics/digl.ppm
    graphics/digm.ppm
    graphics/dign.ppm
    graphics/digo.ppm
    graphics/digp.ppm
    graphics/digq.ppm
    graphics/digr.ppm
    graphics/digs.ppm
    graphics/digt.ppm
    graphics/digu.ppm
    graphics/digv.ppm
    graphics/digw.ppm
    graphics/digx.ppm
    graphics/digy.ppm
    graphics/digz.ppm
    graphics/dig45.ppm
    graphics/dig46.ppm
    graphics/dig47.ppm
    graphics/dig58.ppm
    graphics/dig91.ppm
    graphics/dig93.ppm
    graphics/stbr123.ppm
    graphics/stbr124.ppm
    graphics/stbr125.ppm
    graphics/stbr126.ppm
    graphics/stbr127.ppm
    graphics/boxul.ppm
    graphics/boxuc.ppm
    graphics/boxur.ppm
    graphics/boxcl.ppm
    graphics/boxcc.ppm
    graphics/boxcr.ppm
    graphics/boxll.ppm
    graphics/boxlc.ppm
    graphics/boxlr.ppm
    graphics/stkeys6.ppm
    graphics/stkeys7.ppm
    graphics/stkeys8.ppm
    graphics/stcfn096.ppm
    graphics/m_butt1.ppm
    graphics/m_butt2.ppm
    graphics/m_colors.ppm
    graphics/m_palno.ppm
    graphics/m_palsel.ppm
    graphics/m_vbox.ppm
    graphics/cross1.ppm
    graphics/cross2.ppm
    graphics/cross3.ppm
)

set(FLATS
    flats/-n0_tex-.ppm
)

set(SPRITES
    sprites/tnt1a0.ppm
)
set(SPRITEP "0,0,sprites/tnt1a0.ppm")

set(SPRITES
    ${SPRITES}
    sprites/dogsa1.ppm
    sprites/dogsa2.ppm
    sprites/dogsa3.ppm
    sprites/dogsa4.ppm
    sprites/dogsa5.ppm
    sprites/dogsa6.ppm
    sprites/dogsa7.ppm
    sprites/dogsa8.ppm
    sprites/dogsb1.ppm
    sprites/dogsb2.ppm
    sprites/dogsb3.ppm
    sprites/dogsb4.ppm
    sprites/dogsb5.ppm
    sprites/dogsb6.ppm
    sprites/dogsb7.ppm
    sprites/dogsb8.ppm
    sprites/dogsc1.ppm
    sprites/dogsc2.ppm
    sprites/dogsc3.ppm
    sprites/dogsc4.ppm
    sprites/dogsc5.ppm
    sprites/dogsc6.ppm
    sprites/dogsc7.ppm
    sprites/dogsc8.ppm
    sprites/dogsd1.ppm
    sprites/dogsd2.ppm
    sprites/dogsd3.ppm
    sprites/dogsd4.ppm
    sprites/dogsd5.ppm
    sprites/dogsd6.ppm
    sprites/dogsd7.ppm
    sprites/dogsd8.ppm
    sprites/dogse1.ppm
    sprites/dogse2.ppm
    sprites/dogse3.ppm
    sprites/dogse4.ppm
    sprites/dogse5.ppm
    sprites/dogse6.ppm
    sprites/dogse7.ppm
    sprites/dogse8.ppm
    sprites/dogsf1.ppm
    sprites/dogsf2.ppm
    sprites/dogsf3.ppm
    sprites/dogsf4.ppm
    sprites/dogsf5.ppm
    sprites/dogsf6.ppm
    sprites/dogsf7.ppm
    sprites/dogsf8.ppm
    sprites/dogsg1.ppm
    sprites/dogsg2.ppm
    sprites/dogsg3.ppm
    sprites/dogsg4.ppm
    sprites/dogsg5.ppm
    sprites/dogsg6.ppm
    sprites/dogsg7.ppm
    sprites/dogsg8.ppm
    sprites/dogsh1.ppm
    sprites/dogsh2.ppm
    sprites/dogsh3.ppm
    sprites/dogsh4.ppm
    sprites/dogsh5.ppm
    sprites/dogsh6.ppm
    sprites/dogsh7.ppm
    sprites/dogsh8.ppm
    sprites/dogsi0.ppm
    sprites/dogsj0.ppm
    sprites/dogsk0.ppm
    sprites/dogsl0.ppm
    sprites/dogsm0.ppm
    sprites/dogsn0.ppm
)
set(SPRITEP
    ${SPRITEP}
    "33,66,sprites/dogsa1.ppm"
    "33,66,sprites/dogsa2.ppm"
    "33,66,sprites/dogsa3.ppm"
    "33,66,sprites/dogsa4.ppm"
    "33,66,sprites/dogsa5.ppm"
    "33,66,sprites/dogsa6.ppm"
    "33,66,sprites/dogsa7.ppm"
    "33,66,sprites/dogsa8.ppm"
    "33,66,sprites/dogsb1.ppm"
    "33,66,sprites/dogsb2.ppm"
    "33,66,sprites/dogsb3.ppm"
    "33,66,sprites/dogsb4.ppm"
    "33,66,sprites/dogsb5.ppm"
    "33,66,sprites/dogsb6.ppm"
    "33,66,sprites/dogsb7.ppm"
    "33,66,sprites/dogsb8.ppm"
    "33,66,sprites/dogsc1.ppm"
    "33,66,sprites/dogsc2.ppm"
    "33,66,sprites/dogsc3.ppm"
    "33,66,sprites/dogsc4.ppm"
    "33,66,sprites/dogsc5.ppm"
    "33,66,sprites/dogsc6.ppm"
    "33,66,sprites/dogsc7.ppm"
    "33,66,sprites/dogsc8.ppm"
    "33,66,sprites/dogsd1.ppm"
    "33,66,sprites/dogsd2.ppm"
    "33,66,sprites/dogsd3.ppm"
    "33,66,sprites/dogsd4.ppm"
    "33,66,sprites/dogsd5.ppm"
    "33,66,sprites/dogsd6.ppm"
    "33,66,sprites/dogsd7.ppm"
    "33,66,sprites/dogsd8.ppm"
    "33,66,sprites/dogse1.ppm"
    "33,66,sprites/dogse2.ppm"
    "33,66,sprites/dogse3.ppm"
    "33,66,sprites/dogse4.ppm"
    "33,66,sprites/dogse5.ppm"
    "33,66,sprites/dogse6.ppm"
    "33,66,sprites/dogse7.ppm"
    "33,66,sprites/dogse8.ppm"
    "33,66,sprites/dogsf1.ppm"
    "33,66,sprites/dogsf2.ppm"
    "33,66,sprites/dogsf3.ppm"
    "33,66,sprites/dogsf4.ppm"
    "33,66,sprites/dogsf5.ppm"
    "33,66,sprites/dogsf6.ppm"
    "33,66,sprites/dogsf7.ppm"
    "33,66,sprites/dogsf8.ppm"
    "33,66,sprites/dogsg1.ppm"
    "33,66,sprites/dogsg2.ppm"
    "33,66,sprites/dogsg3.ppm"
    "33,66,sprites/dogsg4.ppm"
    "33,66,sprites/dogsg5.ppm"
    "33,66,sprites/dogsg6.ppm"
    "33,66,sprites/dogsg7.ppm"
    "33,66,sprites/dogsg8.ppm"
    "33,66,sprites/dogsh1.ppm"
    "33,66,sprites/dogsh2.ppm"
    "33,66,sprites/dogsh3.ppm"
    "33,66,sprites/dogsh4.ppm"
    "33,66,sprites/dogsh5.ppm"
    "33,66,sprites/dogsh6.ppm"
    "33,66,sprites/dogsh7.ppm"
    "33,66,sprites/dogsh8.ppm"
    "33,67,sprites/dogsi0.ppm"
    "33,67,sprites/dogsj0.ppm"
    "33,67,sprites/dogsk0.ppm"
    "33,67,sprites/dogsl0.ppm"
    "33,68,sprites/dogsm0.ppm"
    "33,69,sprites/dogsn0.ppm"
)

set(SPRITES
    ${SPRITES}
    sprites/pls1a0.ppm
    sprites/pls1b0.ppm
    sprites/pls1c0.ppm
    sprites/pls1d0.ppm
    sprites/pls1e0.ppm
    sprites/pls1f0.ppm
    sprites/pls1g0.ppm
    sprites/pls2a0.ppm
    sprites/pls2b0.ppm
    sprites/pls2c0.ppm
    sprites/pls2d0.ppm
    sprites/pls2e0.ppm
)
set(SPRITEP
    ${SPRITEP}
    "9,11,sprites/pls1a0.ppm"
    "8,11,sprites/pls1b0.ppm"
    "9,11,sprites/pls1c0.ppm"
    "8,11,sprites/pls1d0.ppm"
    "16,27,sprites/pls1e0.ppm"
    "16,27,sprites/pls1f0.ppm"
    "18,27,sprites/pls1g0.ppm"
    "9,11,sprites/pls2a0.ppm"
    "8,13,sprites/pls2b0.ppm"
    "11,18,sprites/pls2c0.ppm"
    "16,27,sprites/pls2d0.ppm"
    "18,27,sprites/pls2e0.ppm"
)

set(WAD_SRC 
    ${PALETTE}
    ${LUMPS}
    ${COLORMAPS}
    ${TABLES}
    ${SOUNDS}
    ${GRAPHICS}
    ${FLATS}
    ${SPRITES}
)

set(WAD_CMDLINE 
    -palette ${PALETTE}
    -lumps ${LUMPS}
    -marker C_START -lumps ${COLORMAPS} -marker C_END
    -marker B_START -lumps ${TABLES} -marker B_END
    -sounds ${SOUNDS}
    -graphics ${GRAPHICS}
    -marker FF_START -flats ${FLATS} -marker FF_END
    -marker SS_START -sprites ${SPRITEP} -marker SS_END
)

add_custom_command(
    OUTPUT "${WAD_DATA_PATH}"
    COMMAND "${RDATAWAD_COMMAND}" -I "${CMAKE_CURRENT_SOURCE_DIR}" ${WAD_CMDLINE} -o "${WAD_DATA_PATH}"
    DEPENDS ${RDATAWAD_DEPENDS} ${WAD_SRC}
)
add_custom_target(prboomwad DEPENDS ${WAD_DATA_PATH})
install(FILES ${WAD_DATA_PATH} DESTINATION ${PRBOOMDATADIR} COMPONENT "PrBoom-Plus internal WAD")

if(VITA)
    # Match the original Vita release layout. doom1.wad is intentionally not
    # included; it remains optional user-owned game data.
    set(VITA_DATA_STAGE "${CMAKE_BINARY_DIR}/vita-data")
    set(VITA_DATA_ZIP "${CMAKE_BINARY_DIR}/data.zip")
    set(VITA_DATA_CONFIG "${CMAKE_SOURCE_DIR}/data/vita/prboom-plus.cfg")
    set(VITA_DATA_SOUNDFONT "${CMAKE_SOURCE_DIR}/data/vita/TimGM6mb.sf2"
        CACHE FILEPATH "Optional default FluidSynth soundfont for data.zip")
    set(VITA_FREEDOOM_DIR "" CACHE PATH
        "Optional directory containing Freedoom IWADs and attribution files")
    set(VITA_FREEDOOM_FILES)
    if(VITA_FREEDOOM_DIR)
        set(VITA_FREEDOOM1_WAD "${VITA_FREEDOOM_DIR}/freedoom1.wad")
        set(VITA_FREEDOOM2_WAD "${VITA_FREEDOOM_DIR}/freedoom2.wad")
        set(VITA_FREEDOOM_COPYING "${VITA_FREEDOOM_DIR}/COPYING.txt")
        set(VITA_FREEDOOM_CREDITS "${VITA_FREEDOOM_DIR}/CREDITS.txt")
        set(VITA_FREEDOOM_MUSIC_CREDITS "${VITA_FREEDOOM_DIR}/CREDITS-MUSIC.txt")
        set(VITA_FREEDOOM_FILES
            "${VITA_FREEDOOM1_WAD}"
            "${VITA_FREEDOOM2_WAD}"
            "${VITA_FREEDOOM_COPYING}"
            "${VITA_FREEDOOM_CREDITS}"
            "${VITA_FREEDOOM_MUSIC_CREDITS}"
        )
        foreach(VITA_FREEDOOM_FILE IN LISTS VITA_FREEDOOM_FILES)
            if(NOT EXISTS "${VITA_FREEDOOM_FILE}")
                message(FATAL_ERROR
                    "VITA_FREEDOOM_DIR is missing required file: ${VITA_FREEDOOM_FILE}")
            endif()
        endforeach()
        message(STATUS "Bundling Freedoom IWADs from ${VITA_FREEDOOM_DIR}")
    endif()
    set(VITA_ZIP_TOOL "" CACHE FILEPATH "ZIP archiver used to create Vita data.zip")
    if(NOT VITA_ZIP_TOOL)
        find_program(VITA_ZIP_TOOL
            NAMES 7z 7z.exe 7zz 7zz.exe
            HINTS
                "$ENV{MSYS2_ROOT}/usr/lib/p7zip"
                "$ENV{MSYS2_ROOT}/usr/bin"
                "/c/msys64/usr/lib/p7zip"
                "/c/msys64/usr/bin"
                "/c/devkitPro/msys2/usr/lib/p7zip"
                "/c/devkitPro/msys2/usr/bin"
                "C:/msys64/usr/lib/p7zip"
                "C:/msys64/usr/bin"
                "C:/devkitPro/msys2/usr/lib/p7zip"
                "C:/devkitPro/msys2/usr/bin"
        )
    endif()
    if(NOT VITA_ZIP_TOOL)
        foreach(VITA_ZIP_CANDIDATE
            "/c/msys64/usr/lib/p7zip/7z.exe"
            "/c/msys64/usr/bin/7z.exe"
            "/c/devkitPro/msys2/usr/lib/p7zip/7z.exe"
            "/c/devkitPro/msys2/usr/bin/7z.exe"
        )
            if(EXISTS "${VITA_ZIP_CANDIDATE}")
                set(VITA_ZIP_TOOL "${VITA_ZIP_CANDIDATE}")
                break()
            endif()
        endforeach()
    endif()
    if(NOT VITA_ZIP_TOOL)
        message(FATAL_ERROR
            "A ZIP archiver is required to generate data.zip. "
            "Install 7-Zip in MSYS2 or set VITA_ZIP_TOOL to 7z."
        )
    endif()
    get_filename_component(VITA_ZIP_TOOL_DIR "${VITA_ZIP_TOOL}" DIRECTORY)

    set(VITA_DATA_ZIP_DEPENDS
        "${WAD_DATA_PATH}"
        "${VITA_DATA_CONFIG}"
    )
    if(VITA_FREEDOOM_DIR)
        list(APPEND VITA_DATA_ZIP_DEPENDS ${VITA_FREEDOOM_FILES})
    endif()
    set(VITA_DATA_ZIP_COMMANDS
        COMMAND "${CMAKE_COMMAND}" -E remove_directory "${VITA_DATA_STAGE}"
        COMMAND "${CMAKE_COMMAND}" -E remove -f "${VITA_DATA_ZIP}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${VITA_DATA_STAGE}/data/PrBoom++/iwads"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${VITA_DATA_STAGE}/data/PrBoom++/pwads"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${WAD_DATA_PATH}" "${VITA_DATA_STAGE}/data/PrBoom++/prboom-plus.wad"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${VITA_DATA_CONFIG}" "${VITA_DATA_STAGE}/data/PrBoom++/prboom-plus.cfg"
    )
    if(VITA_FREEDOOM_DIR)
        list(APPEND VITA_DATA_ZIP_COMMANDS
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${VITA_DATA_STAGE}/data/PrBoom++/licenses"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${VITA_FREEDOOM1_WAD}"
                "${VITA_DATA_STAGE}/data/PrBoom++/iwads/freedoom1.wad"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${VITA_FREEDOOM2_WAD}"
                "${VITA_DATA_STAGE}/data/PrBoom++/iwads/freedoom2.wad"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${VITA_FREEDOOM_COPYING}"
                "${VITA_DATA_STAGE}/data/PrBoom++/licenses/Freedoom-COPYING.txt"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${VITA_FREEDOOM_CREDITS}"
                "${VITA_DATA_STAGE}/data/PrBoom++/licenses/Freedoom-CREDITS.txt"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${VITA_FREEDOOM_MUSIC_CREDITS}"
                "${VITA_DATA_STAGE}/data/PrBoom++/licenses/Freedoom-CREDITS-MUSIC.txt"
        )
    endif()
    if(EXISTS "${VITA_DATA_SOUNDFONT}")
        list(APPEND VITA_DATA_ZIP_DEPENDS "${VITA_DATA_SOUNDFONT}")
        list(APPEND VITA_DATA_ZIP_COMMANDS
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${VITA_DATA_SOUNDFONT}" "${VITA_DATA_STAGE}/data/PrBoom++/TimGM6mb.sf2"
        )
    else()
        message(STATUS
            "TimGM6mb.sf2 not found; data.zip will omit the optional FluidSynth soundfont"
        )
    endif()

    add_custom_command(
        OUTPUT "${VITA_DATA_ZIP}"
        ${VITA_DATA_ZIP_COMMANDS}
        COMMAND "${CMAKE_COMMAND}" -E chdir "${VITA_DATA_STAGE}"
            "${CMAKE_COMMAND}" -E env
                "PATH=${VITA_ZIP_TOOL_DIR}:/usr/bin"
                "${VITA_ZIP_TOOL}" a -tzip "${VITA_DATA_ZIP}" data
        # The stage directory is deliberately removed by the first command,
        # so it cannot be the custom command's working directory: the shell
        # would try to cd there before CMake recreates it.
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        DEPENDS ${VITA_DATA_ZIP_DEPENDS}
        COMMENT "Packaging Vita data.zip"
        VERBATIM
    )
    add_custom_target(vita-data-zip ALL DEPENDS "${VITA_DATA_ZIP}")
endif()
