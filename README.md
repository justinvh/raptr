# Raptr - A Dinosaur Platformer - 0.1 UNRELEASED

You're a dinosaur without feathers struggling to understand your place in the world. The humans see you as a threat and a form of entertainment. Maybe it's time you escape. Maybe it's time you find a way out of this all.

## Development Updates

This project is maintained at [GitHub](https://github.com/justinvh/raptr). Additional developer insight and blog can be found at [https://vh.io/](https://vh.io).

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

This project is currently being built in the Windows environment using `vcpkg` and `cmake`. A controller (such as an XInput device) must be plugged in to play

```
$ %USERPROFILE%\vcpkg\vcpkg.exe install sdl2:x64-windows sdl2_image:x64-windows picojson:x64-windows
$ git clone https://github.com/justinvh/raptr.git
$ cd raptr
$ mkdir build
$ cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE:PATH="%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake" -G "Visual Studio 15 2017 Win64" ..
```

You can now use Visual Studio 2017 to compile and build Raptr.

## Building documentation

If you want to build the documentation, then you will need `doxygen` and `graphviz` projects installed. You can then use the `doxygen` target to build documentation.

## Running the tests

None available yet

## Built With

* [SDL2](https://www.libsdl.org/index.php) - Simple DirectMedia Layer software development library
* [SDL2_image](https://www.libsdl.org/projects/SDL_image/) - Image file loading library extensions to SDL2
* [picojson](https://github.com/kazuho/picojson) - A header-file-only, JSON parser serializer in C++
* [aseprite](https://www.aseprite.org/) - Animated sprite editor and pixel art tool
* [musescore](https://musescore.com/) - Create, play, and print beautiful sheet music
* [vcpkg](https://github.com/Microsoft/vcpkg) - C++ Library Manager for Windows, Linux, and MacOS
* [cmake](https://cmake.org/) - Cross-platform build system

