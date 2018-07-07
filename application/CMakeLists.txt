set(RAPTR_APP_CPP
    src/main.cpp)

set(RAPTR_APP_HPP)

# Make the filters available if using VS
set(RAPTR_FILES ${RAPTR_APP_CPP} ${RAPTR_APP_HPP})
foreach(source IN LISTS RAPTR_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "../" "" source_path_msvc "${source_path}")
    string(REPLACE "/" "\\" source_path_msvc "${source_path_msvc}")
    source_group("${source_path_msvc}" FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()

add_executable(raptr ${RAPTR_FILES})
set_property(TARGET raptr PROPERTY PROJECT_LABEL "Application")
target_link_libraries(raptr raptr-engine RaptrDependencies)

# Add compiler requirements
target_compile_features(raptr
    PUBLIC cxx_std_17
    PRIVATE cxx_std_17
)

# 'make install' to the correct locations
install(TARGETS raptr EXPORT RaptrConfig
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)


