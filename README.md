# Raptr - A Dinosaur Platformer

You're a dinosaur without feathers struggling to understand your place in 
the world. The humans see you as a threat and a form of entertainment. Maybe 
it's time you escape. Maybe it's time you find a way out of this all.

This project is maintained at [GitHub](https://github.com/justinvh/raptr). 
Additional developer insight and blog can be found at 
[https://vh.io/](https://vh.io)


![Our Dinosaur Warrior][raptr-idle]

## Installing and Running v0.1-alpha.7

It's pretty dang simple under 64-bit Windows right now.

1. [Download the 0.1-alpha.7 Windows 64-bit Release](https://github.com/justinvh/raptr/releases/download/v0.1-alpha.7/raptr-0.1.0-alpha.7-Release-win64.zip)
2. Extract that anywhere your heart desires.
3. Plug in a 360, GameCube, or Steam controller.
4. Go into the bin/ folder and run `raptr-client.exe`
5. Realize how much of an alpha this is :)

If you get a complaint about redistributables missing, then try downloading the [Visual C++ Redistributable for Visual Studio 2017 - x64](https://aka.ms/vs/15/release/vc_redist.x64.exe)

## Current State: v0.1-alpha.7

Development screenshots are [available in this Imgur Album](https://imgur.com/a/pnREFi5)

![Feature Mash][raptr-0.1-alpha.7]

## Features with some Hyperbole

- A state-of-the-art PHY-101 physics system
- A seamless and fluid split screen local multiplayer
- An immersive and childish dialog and decision making system
- Pixel perfect platforming with that keeps you on the edge of your seat
- A flexible mapping system that makes modding a painless process
- Music that will cut you to your core
- Parallax backgrounds of untold depth

## Building and Running Raptr with CMake

1. [Download the 7-zip archive of Raptr's dependencies from here](https://github.com/justinvh/raptr/releases/download/v0.1-alpha.7/vcpkg-export-20180711-180948.7z)

```
$ git clone https://github.com/justinvh/raptr.git raptr
$ cd raptr
$ 7z e vcpkg-export-20180711-180948.7z
$ cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE:PATH="../vcpkg/scripts/buildsystems/vcpkg.cmake" -G "Visual Studio 15 2017 Win64" ..
```

You can now use Visual Studio 2017 to compile and build Raptr. If you want to manually fetch the dependencies and use vcpkg in its entirety, then [follow the manual build guide](https://github.com/justinvh/raptr/tree/master/doc/BUILDING_WITH_VCPKG.md) in the docs folder.

## Building documentation

If you want to build the documentation, then you will need `doxygen` and 
`graphviz` projects installed. You can then use the `doxygen` target to 
build documentation.

## Running the tests

Raptr is built with Catch2 and CMake's CTest. You can run tests by building the `raptr-tests` component
and then running the `RUN TESTS` target.

## Checklist

To be added

## Built With

* [aseprite](https://www.aseprite.org/) - Animated sprite editor and pixel art tool
* [cmake](https://cmake.org/) - Cross-platform build system
* [crossguid](https://github.com/graeme-hill/crossguid) - Lightweight cross platform C++ GUID/UUID library
* [cxxopts](https://github.com/jarro2783/cxxopts) - Lightweight C++ command line option parser
* [Lua](https://www.lua.org/) - It's got what game scripting craves
* [musescore](https://musescore.com/) - Create, play, and print beautiful sheet music
* [picojson](https://github.com/kazuho/picojson) - A header-file-only, JSON parser serializer in C++
* [SDL2](https://www.libsdl.org/index.php) - Simple DirectMedia Layer software development library
* [SDL2](https://www.libsdl.org/index.php) - Simple DirectMedia Layer software development library
* [SDL2_image](https://www.libsdl.org/projects/SDL_image/) - Image file loading library extensions to SDL2
* [SDL2_TTF](https://www.libsdl.org/projects/SDL_ttf/docs/SDL_ttf.html) - TTF Support in SDL
* [SDL2_net](https://www.libsdl.org/projects/SDL_net) - SDL Networking
* [sol2](https://github.com/ThePhD/sol2) - C++ <-> Lua API wrapper with advanced features and top notch performance
* [spdlog](https://github.com/gabime/spdlog) - Fast C++ logging library
* [tinytoml](https://github.com/mayah/tinytoml) - A header only C++11 library for parsing TOML
* [vcpkg](https://github.com/Microsoft/vcpkg) - C++ Library Manager for Windows, Linux, and MacOS

[raptr-idle]: https://i.imgur.com/sqVdbnN.gif
[raptr-0.1-alpha.7]: https://i.imgur.com/pNqNhiF.gif
