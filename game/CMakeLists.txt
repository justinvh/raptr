set(RAPTR_GAME
    # Aseprite
    aseprite/dialog-simple-sheet.aseprite
    aseprite/raptr.aseprite

    # Backgrounds
    background/nightsky.toml

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

    # Foregrounds
    foreground/nightsky.toml

    # Maps
    maps/demo/demo.aseprite
    maps/demo/demo.json
    maps/demo/demo.png
    maps/demo/map.toml
    maps/demo/map.lua
    maps/e1m1/map.json
    maps/e1m1/map.lua

    # Sound
    sound/opening.mp3

    # Textures
    textures/raptor-spritesheet.png
    textures/raptor.json
    textures/dialog-simple.png
    textures/dialog-simple.json

    # Parallax backgrounds
    textures/background/nightsky/nightsky_1.png
    textures/background/nightsky/nightsky_2.png
    textures/background/nightsky/nightsky_3.png
    textures/background/nightsky/nightsky_4.png
    textures/background/nightsky/nightsky_5.png
    textures/background/nightsky/nightsky_6.png
    textures/background/nightsky/nightsky_7.png
)

add_custom_target(game SOURCES ${RAPTR_GAME})
set_property(TARGET game PROPERTY PROJECT_LABEL "Game Resources")
set_target_properties(game PROPERTIES FOLDER "Application")
foreach(source IN LISTS RAPTR_GAME)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source}")
    source_group("${source_path_msvc}" FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()
