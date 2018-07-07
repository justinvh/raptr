# Findtinytoml.cmake
#
# Finds the rapidjson library
#
# This will define the following variables
#
#    TINYTOML_FOUND
#    TINYTOML_INCLUDE_DIRS
#
# and the following imported targets
#
#     tinytoml::tinytoml
#

find_package(PkgConfig QUIET)
pkg_check_modules(tinytoml QUIET tinytoml)

find_path(TINYTOML_INCLUDE_DIR
    NAMES toml.h
    PATHS ${PC_TINYTOML_INCLUDE_DIRS}
    PATH_SUFFIXES toml
)

set(TINYTOML_VERSION ${PC_TINYTOML_VERSION})

mark_as_advanced(TINYTOML_FOUND TINYTOML_INCLUDE_DIR TINYTOML_VERSION)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(tinytoml
    REQUIRED_VARS TINYTOML_INCLUDE_DIR
    VERSION_VAR TINYTOML_VERSION
)

if(TINYTOML_FOUND)
    set(TINYTOML_INCLUDE_DIRS ${TINYTOML_INCLUDE_DIR})
endif()

if(TINYTOML_FOUND AND NOT TARGET tinytoml::tinytoml)
    add_library(tinytoml::tinytoml INTERFACE IMPORTED)
    set_target_properties(tinytoml::tinytoml PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${TINYTOML_INCLUDE_DIR}"
    )
endif()
