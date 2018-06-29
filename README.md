# Raptr - A Dinosaur Platformer

You're a dinosaur without feathers struggling to understand your place in 
the world. The humans see you as a threat and a form of entertainment. Maybe 
it's time you escape. Maybe it's time you find a way out of this all.

![Our Dinosaur Warrior][raptr-idle]

## Development Updates

This project is maintained at [GitHub](https://github.com/justinvh/raptr). 
Additional developer insight and blog can be found at 
[https://vh.io/](https://vh.io).

## Getting Started

These instructions will get you a copy of the project up and running on your 
local machine for development and testing purposes. See deployment for notes 
on how to deploy the project on a live system.

### Prerequisites - Windows 10

This project is currently being built in the Windows environment using `vcpkg` 
and `cmake`. A controller (such as an XInput device) must be plugged 
in to play.

### Patch fmt package

Unfortunately, the fmt package is backwards incompatible with spdlog at the moment, so you will have to checkout the port
at a specific version:

```
$ cd %USERPROFILE%\vcpkg
$ git checkout 178517052f42d428bb2f304946e635d3c1f318e9 -- ports/fmt
```

### Continue on!

You will want to first install the dependencies:

```
$ %USERPROFILE%\vcpkg\vcpkg.exe --triplet x64-windows install sdl2 sdl2-image sdl2-ttf picojson cxxopts spdlog tinytoml
```

Now you can specify the toolchain file and continue on:

```
$ git clone https://github.com/justinvh/raptr.git raptr
$ cd raptr/build
$ cmake -DCMAKE_TOOLCHAIN_FILE:PATH="%USERPROFILE%/vcpkg/scripts/buildsystems/vcpkg.cmake" -G "Visual Studio 15 2017 Win64" ..
```

Alternatively, if you don't want to use vcpkg and are running Windows 10 64-bit, then you can [download a 7zip export of the dependencies](https://drive.google.com/open?id=1XUcirZww859o7s_iTD9b9Xu2DlB8RYMK), extract into the checked out project, and:

```
$ git clone https://github.com/justinvh/raptr.git raptr
$ cd raptr
$ 7z e vcpkg-export.7z
$ cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE:PATH="../vcpkg/scripts/buildsystems/vcpkg.cmake" -G "Visual Studio 15 2017 Win64" ..
```

You can now use Visual Studio 2017 to compile and build Raptr.

## Building documentation

If you want to build the documentation, then you will need `doxygen` and 
`graphviz` projects installed. You can then use the `doxygen` target to 
build documentation.

## Running the tests

None available yet

## Checklist

To be added

## Built With

* [aseprite](https://www.aseprite.org/) - Animated sprite editor and pixel art tool
* [cmake](https://cmake.org/) - Cross-platform build system
* [cxxopts](https://github.com/jarro2783/cxxopts) - Lightweight C++ command line option parser
* [musescore](https://musescore.com/) - Create, play, and print beautiful sheet music
* [picojson](https://github.com/kazuho/picojson) - A header-file-only, JSON parser serializer in C++
* [SDL2](https://www.libsdl.org/index.php) - Simple DirectMedia Layer software development library
* [SDL2_image](https://www.libsdl.org/projects/SDL_image/) - Image file loading library extensions to SDL2
* [SDL2_TTF](https://www.libsdl.org/projects/SDL_ttf/docs/SDL_ttf.html) - TTF Support in SDL
* [spdlog](https://github.com/gabime/spdlog) - Fast C++ logging library
* [tinytoml](https://github.com/mayah/tinytoml) - A header only C++11 library for parsing TOML
* [vcpkg](https://github.com/Microsoft/vcpkg) - C++ Library Manager for Windows, Linux, and MacOS

[raptr-idle]: https://i.imgur.com/sqVdbnN.gif
