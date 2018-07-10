find_package(Catch2 REQUIRED)     
include(ParseAndAddCatchTests)

set(TEST_SOURCES simple.cpp)
add_executable(raptr-tests ${TEST_SOURCES})
set_property(TARGET raptr-tests PROPERTY PROJECT_LABEL "Engine Tests")
set_target_properties(raptr-tests PROPERTIES FOLDER "Support")
target_link_libraries(raptr-tests RaptrDependencies Catch2::Catch raptr-engine)

ParseAndAddCatchTests(raptr-tests RAPTR_TESTS)

# Make the filters available if using VS and add the executable
foreach(source IN LISTS RAPTR_TESTS)
    get_filename_component(source_path "${source}" PATH)
    get_filename_component(name_we "${source}" NAME_WE)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()
