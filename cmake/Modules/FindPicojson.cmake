# FindPicojson.cmake
#
# Finds the rapidjson library
#
# This will define the following variables
#
#    PICOJSON_FOUND
#    PICOJSON_INCLUDE_DIRS
#
# and the following imported targets
#
#     Picojson::Picojson
#
# Author: Pablo Arias - pabloariasal@gmail.com

find_package(PkgConfig QUIET)
pkg_check_modules(Picojson QUIET Picojson)

find_path(PICOJSON_INCLUDE_DIR
    NAMES picojson.h
    PATHS ${PC_PICOJSON_INCLUDE_DIRS}
    PATH_SUFFIXES picojson
)

set(PICOJSON_VERSION ${PC_PICOJSON_VERSION})

mark_as_advanced(PICOJSON_FOUND PICOJSON_INCLUDE_DIR PICOJSON_VERSION)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Picojson
    REQUIRED_VARS PICOJSON_INCLUDE_DIR
    VERSION_VAR PICOJSON_VERSION
)

if(PICOJSON_FOUND)
    set(PICOJSON_INCLUDE_DIRS ${PICOJSON_INCLUDE_DIR})
endif()

if(PICOJSON_FOUND AND NOT TARGET Picojson::Picojson)
    add_library(Picojson::Picojson INTERFACE IMPORTED)
    set_target_properties(Picojson::Picojson PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PICOJSON_INCLUDE_DIR}"
    )
endif()
