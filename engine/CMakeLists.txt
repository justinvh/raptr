# RAPTR, the best around.

set(RAPTR_CPP
    # Common sources
    src/common/filesystem.cpp
    src/common/logger.cpp
    src/common/clock.cpp

    # Game sources
    src/game/actor.cpp
    src/game/character.cpp
    src/game/console.cpp
    src/game/entity.cpp
    src/game/game.cpp
    src/game/map.cpp
    src/game/trigger.cpp

    # Lua bindings
    src/game/lua_game.cpp

    # Input sources
    src/input/controller.cpp

    # Networking
    src/network/client.cpp
    src/network/server.cpp
    src/network/snapshot.cpp

    # Renderer sources
    src/renderer/camera.cpp
    src/renderer/parallax.cpp
    src/renderer/renderer.cpp
    src/renderer/sprite.cpp
    src/renderer/shader.cpp

    # Sound sources
    src/sound/sound.cpp

    # UI sources
    src/ui/dialog.cpp
    src/ui/font.cpp
)

set(RAPTR_HPP
    # Common headers
    include/raptr/common/clock.hpp
    include/raptr/common/rect.hpp
    include/raptr/common/rtree.hpp
    include/raptr/common/filesystem.hpp
    include/raptr/common/logging.hpp

    # Game headers
    include/raptr/game/actor.hpp
    include/raptr/game/character.hpp
    include/raptr/game/console.hpp
    include/raptr/game/entity.hpp
    include/raptr/game/game.hpp
    include/raptr/game/map.hpp
    include/raptr/game/trigger.hpp

    # Input headers
    include/raptr/input/controller.hpp

    # Networking
    include/raptr/network/client.hpp
    include/raptr/network/server.hpp
    include/raptr/network/snapshot.hpp

    # Renderer headers
    include/raptr/renderer/camera.hpp
    include/raptr/renderer/parallax.hpp
    include/raptr/renderer/renderer.hpp
    include/raptr/renderer/sprite.hpp
    include/raptr/renderer/shader.hpp

    # Sound headers
    include/raptr/sound/sound.hpp

    # UI headers
    include/raptr/ui/ui.hpp
    include/raptr/ui/dialog.hpp
    include/raptr/ui/font.hpp

    # Top-Level
    include/raptr/config.hpp
)

set(RAPTR_ENGINE ${RAPTR_HPP} ${RAPTR_CPP})

add_library(raptr-engine STATIC ${RAPTR_ENGINE})
set_property(TARGET raptr-engine PROPERTY PROJECT_LABEL "Engine")
set_target_properties(raptr-engine PROPERTIES VERSION ${PROJECT_VERSION})

# Make the filters available if using VS
foreach(source IN LISTS RAPTR_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()


target_include_directories(raptr-engine
	PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
	       "$<INSTALL_INTERFACE:include>"
)

target_link_libraries(raptr-engine PRIVATE RaptrDependencies)

# Add compiler requirements
target_compile_features(raptr-engine
    PUBLIC cxx_std_17
    PRIVATE cxx_std_17
)

# 'make install' to the correct locations
install(TARGETS raptr-engine EXPORT RaptrConfig
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

