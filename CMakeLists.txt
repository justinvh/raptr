cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# RAPTR, the best around.
project(raptr
        VERSION 0.1
        LANGUAGES CXX)

# Include additional custom cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

# Targets that we develop under RAPTR
enable_testing()

set(RAPTR_CPP
    # Game sources
    src/raptr/game/character.cpp
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

    # Top-level sources
    src/raptr/main.cpp
    src/raptr/config.cpp
)

set(HEADER_PREFIX)

set(RAPTR_HPP
    # Common headers
    include/raptr/common/rect.hpp
    include/raptr/common/rtree.hpp

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

    # Top-Level
    include/raptr/config.hpp
)

set(RAPTR_CMAKE
    # Modules
    cmake/Modules/FindSDL2_Image.cmake
    cmake/Modules/FindPicojson.cmake
)

set(RAPTR_GAME
    # Game modules
    game/controls/gamecontrollerdb.txt

    # Sound
    game/sound/opening.mp3

    # Textures
    game/textures/fire-sheet.png
    game/textures/fire.json
    game/textures/fire.png
    game/textures/platform.png
    game/textures/platform.png
    game/textures/raptor-spritesheet.png
    game/textures/raptr.json
)

set(RAPTR_SOURCES ${RAPTR_HPP} ${RAPTR_CPP})
set(RAPTR_FILES ${RAPTR_SOURCES} ${RAPTR_CMAKE} ${RAPTR_GAME})

# Create our Raptr target executable
add_executable(raptr ${RAPTR_SOURCES})

# Make the filters available if using VS
foreach(source IN LISTS RAPTR_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()

# Imported target: SDL2
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image REQUIRED)

add_library(sdl2 INTERFACE IMPORTED)
set_property(TARGET sdl2 PROPERTY
    INTERFACE_LINK_LIBRARIES ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
set_property(TARGET sdl2 PROPERTY 
    INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})

target_include_directories(raptr PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)

target_link_libraries(raptr sdl2)

# Add compiler requirements
target_compile_features(raptr
    PUBLIC cxx_auto_type
    PRIVATE cxx_variadic_templates)

# 'make install' to the correct locations
install(TARGETS raptr EXPORT RaptrConfig
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

# Make the project importable
install(EXPORT RaptrConfig DESTINATION share/Raptr/cmake)