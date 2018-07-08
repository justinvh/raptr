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

add_executable(raptr_server ${RAPTR_SERVER_CPP})
set_property(TARGET raptr_server PROPERTY PROJECT_LABEL "Raptr Main (Server)")
target_link_libraries(raptr_server raptr-engine RaptrDependencies)
target_compile_features(raptr_server PUBLIC cxx_std_17 PRIVATE cxx_std_17)
install(TARGETS raptr_server EXPORT RaptrConfig
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

add_executable(raptr_client ${RAPTR_CLIENT_CPP})
set_property(TARGET raptr_client PROPERTY PROJECT_LABEL "Raptr Main (Client)")
target_link_libraries(raptr_client raptr-engine RaptrDependencies)
target_compile_features(raptr_client PUBLIC cxx_std_17 PRIVATE cxx_std_17)
install(TARGETS raptr_client EXPORT RaptrConfig
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)
