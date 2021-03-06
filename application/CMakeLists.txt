set(RAPTR_SERVER_CPP src/main_server.cpp)
set(RAPTR_CLIENT_CPP src/main_client.cpp)

set(RAPTR_FILES ${RAPTR_SERVER_CPP} ${RAPTR_CLIENT_CPP})

# Make the filters available if using VS
set(RAPTR_FILES ${RAPTR_APP_CPP} ${RAPTR_APP_HPP})
foreach(source IN LISTS RAPTR_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "../" "" source_path_msvc "${source_path}")
    string(REPLACE "/" "\\" source_path_msvc "${source_path_msvc}")
    source_group("${source_path_msvc}" FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()

add_executable(raptr-server ${RAPTR_SERVER_CPP})
set_property(TARGET raptr-server PROPERTY PROJECT_LABEL "Raptr Main (Server)")
set_target_properties(raptr-server PROPERTIES FOLDER "Application")

target_link_libraries(raptr-server raptr-engine RaptrDependencies)
target_compile_features(raptr-server PUBLIC cxx_std_17 PRIVATE cxx_std_17)
install(TARGETS raptr-server EXPORT RaptrConfig
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

target_compile_definitions(raptr-server PUBLIC -D_ITERATOR_DEBUG_LEVEL=0 -D_ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH)

add_executable(raptr-client ${RAPTR_CLIENT_CPP})
set_property(TARGET raptr-client PROPERTY PROJECT_LABEL "Raptr Main (Client)")
set_target_properties(raptr-client PROPERTIES FOLDER "Application")
target_link_libraries(raptr-client raptr-engine RaptrDependencies)
target_compile_features(raptr-client PUBLIC cxx_std_17 PRIVATE cxx_std_17)
install(TARGETS raptr-client EXPORT RaptrConfig
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

target_compile_definitions(raptr-client PUBLIC -D_ITERATOR_DEBUG_LEVEL=0 -D_ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH)
