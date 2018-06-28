# Findcxxopts.cmake
#
# Finds the rapidjson library
#
# This will define the following variables
#
#    CXXOPTS_FOUND
#    CXXOPTS_INCLUDE_DIRS
#
# and the following imported targets
#
#     cxxopts::cxxopts
#

find_package(PkgConfig)
pkg_check_modules(cxxopts QUIET cxxopts)

find_path(CXXOPTS_INCLUDE_DIR
    NAMES cxxopts.hpp
    PATHS ${PC_CXXOPTS_INCLUDE_DIRS}
    PATH_SUFFIXES cxxopts
)

set(CXXOPTS_VERSION ${PC_CXXOPTS_VERSION})

mark_as_advanced(CXXOPTS_FOUND CXXOPTS_INCLUDE_DIR CXXOPTS_VERSION)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cxxopts
    REQUIRED_VARS CXXOPTS_INCLUDE_DIR
    VERSION_VAR CXXOPTS_VERSION
)

if(CXXOPTS_FOUND)
    set(CXXOPTS_INCLUDE_DIRS ${CXXOPTS_INCLUDE_DIR})
endif()

if(CXXOPTS_FOUND AND NOT TARGET cxxopts::cxxopts)
    add_library(cxxopts::cxxopts INTERFACE IMPORTED)
    set_target_properties(cxxopts::cxxopts PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CXXOPTS_INCLUDE_DIR}"
    )
endif()
