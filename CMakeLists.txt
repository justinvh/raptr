cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# RAPTR, the best around.
project(raptr
        VERSION 0.1
        LANGUAGES CXX)

# Include additional custom cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

# Options for the project
option(BUILD_DOCS "Build documentation" ON)
option(BUILD_TESTS "Build tests" ON)

# Imported target: SDL2
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_TTF REQUIRED)
find_package(SDL2_Net REQUIRED)
find_package(Picojson REQUIRED)
find_package(cxxopts REQUIRED)
find_package(spdlog REQUIRED)     
find_package(fmt REQUIRED)     
find_package(tinytoml REQUIRED)     
find_package(crossguid CONFIG REQUIRED)

add_library(RaptrDependencies INTERFACE)

target_link_libraries(RaptrDependencies INTERFACE
    cxxopts::cxxopts
    SDL2::SDL2
    sdl2_ttf::sdl2_ttf
    Picojson::Picojson
    spdlog::spdlog
    fmt::fmt
    tinytoml::tinytoml
    sdl2_net::sdl2_net
    crossguid
    ${SDL2_LIBRARIES} 
    ${SDL2_IMAGE_LIBRARIES}
)

target_include_directories(RaptrDependencies INTERFACE
    $<TARGET_PROPERTY:cxxopts::cxxopts,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:SDL2::SDL2,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:sdl2_ttf::sdl2_ttf,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:sdl2_net::sdl2_net,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Picojson::Picojson,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:spdlog::spdlog,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:fmt::fmt,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:tinytoml::tinytoml,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:crossguid,INTERFACE_INCLUDE_DIRECTORIES>
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_IMAGE_INCLUDE_DIRS}
)

# Make the project importable
install(EXPORT RaptrConfig DESTINATION share/Raptr/cmake)

# 'make install' to the correct locations
install(TARGETS RaptrDependencies EXPORT RaptrConfig
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)

# Unfortunately, cmake doesn't support this yet
if (MSVC) 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

add_subdirectory(engine)
add_subdirectory(application)
add_subdirectory(game)

# Targets that we develop under RAPTR
if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Check if doxygen is installed and available
if (BUILD_DOCS)
    add_subdirectory(doc)
endif(BUILD_DOCS)

