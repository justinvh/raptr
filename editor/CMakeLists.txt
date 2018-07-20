# RAPTR, but the editor. Still the best around.

set(EDITOR_CPP
    src/main.cpp
    src/mainwindow.cpp
)

set(EDITOR_HPP
    include/raptr/editor/mainwindow.hpp
)

set(EDITOR_ENGINE ${EDITOR_HPP} ${EDITOR_CPP})

add_executable(raptr-editor WIN32 ${EDITOR_ENGINE})
set_property(TARGET raptr-editor PROPERTY PROJECT_LABEL "Editor")
set_target_properties(raptr-editor PROPERTIES VERSION ${PROJECT_VERSION})

# Make the filters available if using VS
foreach(source IN LISTS EDITOR_ENGINE)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()


target_include_directories(raptr-editor
	PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
	       "$<INSTALL_INTERFACE:include>"
)

target_link_libraries(raptr-editor PRIVATE RaptrEditorDependencies)

# Add compiler requirements
target_compile_features(raptr-editor
    PUBLIC cxx_std_17
    PRIVATE cxx_std_17
)

# 'make install' to the correct locations
install(TARGETS raptr-editor EXPORT RaptrConfig
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
