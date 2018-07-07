set(RAPTR_GAME
    # Aseprite
    aseprite/dialog-simple-sheet.aseprite
    aseprite/raptr.aseprite

    # Game modules
    controls/gamecontrollerdb.txt

    # Characters
    characters/raptr.toml

    # Dialog
    dialog/dialog.toml
    dialog/demo/dialog.toml

    # Fonts
    fonts/04b.ttf
    fonts/americancursive.ttf
    fonts/munro_small.ttf
    fonts/fonts.toml

    # Maps
    maps/demo/demo.aseprite
    maps/demo/demo.json
    maps/demo/demo.png
    maps/demo/map.toml

    # Static meshes
    staticmeshes/fire.toml
    staticmeshes/platform.toml
    staticmeshes/staticmeshes.toml

    # Sound
    sound/opening.mp3

    # Textures
    textures/fire-sheet.png
    textures/fire.json
    textures/fire.png
    textures/platform.png
    textures/platform.json
    textures/raptor-spritesheet.png
    textures/raptor.json
    textures/dialog-simple.png
    textures/dialog-simple.json
)

add_custom_target(game SOURCES ${RAPTR_GAME})
set_property(TARGET game PROPERTY PROJECT_LABEL "Game Resources")
foreach(source IN LISTS RAPTR_GAME)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source}")
    source_group("${source_path_msvc}" FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()
