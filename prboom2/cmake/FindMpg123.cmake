#-------------------------------------------------------------------------------
# Find libmpg123.
#
# This module sets:
#   MPG123_FOUND
#   MPG123_INCLUDE_DIRS
#   MPG123_LIBRARIES
#-------------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)

set(_MPG123_HINTS)
if(VITA AND VITASDK)
    list(APPEND _MPG123_HINTS "${VITASDK}/arm-vita-eabi")
endif()
if(MPG123_ROOT)
    list(APPEND _MPG123_HINTS "${MPG123_ROOT}")
endif()

find_path(MPG123_INCLUDE_DIRS
    NAMES mpg123.h
    HINTS ${_MPG123_HINTS}
    PATH_SUFFIXES include
)

find_library(MPG123_LIBRARIES
    NAMES mpg123
    HINTS ${_MPG123_HINTS}
    PATH_SUFFIXES lib
)

find_package_handle_standard_args(
    Mpg123
    DEFAULT_MSG
    MPG123_LIBRARIES
    MPG123_INCLUDE_DIRS
)

mark_as_advanced(MPG123_INCLUDE_DIRS MPG123_LIBRARIES)
