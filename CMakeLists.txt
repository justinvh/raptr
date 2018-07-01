cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# RAPTR, the best around.
project(raptr
        VERSION 0.1
        LANGUAGES CXX)

# Include additional custom cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

# Options for the project
option(BUILD_DOCS "Build documentation" OFF)
option(BUILD_TESTS "Build tests" ON)

# Check if doxygen is installed and available
if (BUILD_DOCS)
    find_package(Doxygen REQUIRED dot)
    if (DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        add_custom_target(doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generarting API documentation with Doxygen"
            VERBATIM)
    else(DOXYGEN_FOUND)
        message("Doxygen is needed to generate doxygen documentation")
    endif(DOXYGEN_FOUND)
endif(BUILD_DOCS)

# Targets that we develop under RAPTR
if (BUILD_TESTS)
	enable_testing()
endif()

set(RAPTR_CPP
    # Common sources
    src/raptr/common/filesystem.cpp
    src/raptr/common/logger.cpp

    # Game sources
    src/raptr/game/character.cpp
    src/raptr/game/entity.cpp
    src/raptr/game/game.cpp

    # Input sources
    src/raptr/input/controller.cpp

    # Map sources

    # Renderer sources
    src/raptr/renderer/renderer.cpp
    src/raptr/renderer/sprite.cpp
    src/raptr/renderer/static_mesh.cpp

    # Sound sources
    src/raptr/sound/sound.cpp

    # UI sources
    src/raptr/ui/dialog.cpp
    src/raptr/ui/font.cpp

    # Top-level sources
    src/raptr/main.cpp
    src/raptr/config.cpp
)

set(RAPTR_HPP
    # Common headers
    include/raptr/common/rect.hpp
    include/raptr/common/rtree.hpp
    include/raptr/common/filesystem.hpp
    include/raptr/common/logging.hpp

    # Game headers
    include/raptr/game/character.hpp
    include/raptr/game/entity.hpp
    include/raptr/game/game.hpp

    # Input headers
    include/raptr/input/controller.hpp

    # Map headers
    include/raptr/map/map.hpp

    # Renderer headers
    include/raptr/renderer/renderer.hpp
    include/raptr/renderer/sprite.hpp
    include/raptr/renderer/static_mesh.hpp

    # Sound headers
    include/raptr/sound/sound.hpp

    # UI headers
    include/raptr/ui/ui.hpp
    include/raptr/ui/dialog.hpp
    include/raptr/ui/font.hpp

    # Top-Level
    include/raptr/config.hpp
)

set(RAPTR_CMAKE
    # Modules
    cmake/Modules/FindSDL2_Image.cmake
    cmake/Modules/FindSDL2_TTF.cmake
    cmake/Modules/FindPicojson.cmake
    cmake/Modules/Findcxxopts.cmake
    cmake/Modules/Findtinytoml.cmake
)

set(RAPTR_GAME
    # Aseprite
    game/aseprite/dialog-simple-sheet.aseprite
    game/aseprite/raptr.aseprite

    # Game modules
    game/controls/gamecontrollerdb.txt

    # Characters
    game/characters/raptr.toml

    # Dialog
    game/dialog/dialog.toml
    game/dialog/demo/dialog.toml

    # Fonts
    game/fonts/04b.ttf
    game/fonts/americancursive.ttf
    game/fonts/munro_small.ttf
    game/fonts/fonts.toml

    # Static meshes
    game/staticmeshes/fire.toml
    game/staticmeshes/platform.toml
    game/staticmeshes/staticmeshes.toml

    # Sound
    game/sound/opening.mp3

    # Textures
    game/textures/fire-sheet.png
    game/textures/fire.json
    game/textures/fire.png
    game/textures/platform.png
    game/textures/platform.json
    game/textures/raptor-spritesheet.png
    game/textures/raptor.json
    game/textures/dialog-simple.png
    game/textures/dialog-simple.json
)

set(RAPTR_SOURCES ${RAPTR_HPP} ${RAPTR_CPP})
set(RAPTR_FILES ${RAPTR_HPP} ${RAPTR_CPP} ${RAPTR_CMAKE} ${RAPTR_GAME})

# Create our Raptr target executable
add_executable(raptr ${RAPTR_FILES})

# Make the filters available if using VS
foreach(source IN LISTS RAPTR_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()

# Imported target: SDL2
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_TTF REQUIRED)
find_package(Picojson REQUIRED)
find_package(cxxopts REQUIRED)
find_package(spdlog REQUIRED)     
find_package(fmt REQUIRED)     
find_package(tinytoml REQUIRED)     

add_library(sdl2 INTERFACE IMPORTED)
set_property(TARGET sdl2 PROPERTY
    INTERFACE_LINK_LIBRARIES ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
set_property(TARGET sdl2 PROPERTY 
    INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})

target_include_directories(raptr 
	PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
	       "$<INSTALL_INTERFACE:include>"
)

target_include_directories(raptr SYSTEM
	INTERFACE ${TINYTOML_INCLUDE_DIRS}
)

target_link_libraries(raptr PRIVATE
	cxxopts::cxxopts 
	SDL2::SDL2 
	sdl2
	sdl2_ttf::sdl2_ttf
	Picojson::Picojson
	spdlog::spdlog
	fmt::fmt
	tinytoml::tinytoml
)

# Add compiler requirements
target_compile_features(raptr
    PUBLIC cxx_std_17
    PRIVATE cxx_std_17
)

# Unfortunately, cmake doesn't support this yet
if (MSVC) 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

# 'make install' to the correct locations
install(TARGETS raptr EXPORT RaptrConfig
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

# Make the project importable
install(EXPORT RaptrConfig DESTINATION share/Raptr/cmake)
