# Raptr - A Dinosaur Platformer

You're a dinosaur without feathers struggling to understand your place in 
the world. The humans see you as a threat and a form of entertainment. Maybe 
it's time you escape. Maybe it's time you find a way out of this all.

This project is maintained at [GitHub](https://github.com/justinvh/raptr). 
Additional developer insight and blog can be found at 
[https://vh.io/](https://vh.io)


![Our Dinosaur Warrior][raptr-idle]

## Installing and Running v0.1-alpha.3

It's pretty dang simple under 64-bit Windows right now.

1. [Download the 0.1-alpha.4 Windows 64-bit Release](https://github.com/justinvh/raptr/releases/download/v0.1-alpha.4/raptr-0.1.0-alpha.4-win64.zip)
2. Extract that anywhere your heart desires.
3. Plug in a 360, GameCube, or Steam controller.
4. Go into the bin/ folder and run raptr.exe
5. Realize how much of an alpha this is :)

## Current State: v0.1-alpha.4

![Feature Mash][raptr-0.1-alpha.4]

## Features with some Hyperbole

- A state-of-the-art PHY-101 physics system
- A seamless and fluid split screen local multiplayer
- An immersive and childish dialog and decision making system
- Pixel perfect platforming with that keeps you on the edge of your seat
- A flexible mapping system that makes modding a painless process
- Music that will cut you to your core
- Parallax backgrounds of untold depth

## Building and Running Raptr with CMake

1. [Download the 7-zip archive of Raptr's dependencies from here](https://github.com/justinvh/raptr/releases/download/v0.1-alpha.4/vcpkg-export-20180707-072728.7z) (or [zip](https://github.com/justinvh/raptr/releases/download/v0.1-alpha.4/vcpkg-export-20180707-072728.zip))

```
$ git clone https://github.com/justinvh/raptr.git raptr
$ cd raptr
$ 7z e vcpkg-export-20180707-072728.7z
$ cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE:PATH="../vcpkg/scripts/buildsystems/vcpkg.cmake" -G "Visual Studio 15 2017 Win64" ..
```

You can now use Visual Studio 2017 to compile and build Raptr.

## Manually Fetching Dependencies

Raptr Depends on the following:

* bzip2
* catch2
* crossguid
* cxxopts
* fmt
* freetype
* libjpeg-turbo
* liblzma
* libpng
* libwebp
* picojson
* sdl2
* sdl2-image
* sdl2-net
* sdl2-ttf
* spdlog
* tiff
* tinytoml
* zlib

Which can be installed with vcpkg.exe with the following caveats:

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
$ %USERPROFILE%\vcpkg\vcpkg.exe --triplet x64-windows install sdl2 sdl2-image sdl2-net sdl2-ttf picojson cxxopts spdlog tinytoml catch2 crossguid
```

### Resume Building
Now you can specify the toolchain file and continue on:

```
$ git clone https://github.com/justinvh/raptr.git raptr
$ cd raptr/build
$ cmake -DCMAKE_TOOLCHAIN_FILE:PATH="%USERPROFILE%/vcpkg/scripts/buildsystems/vcpkg.cmake" -G "Visual Studio 15 2017 Win64" ..
```

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
* [musescore](https://musescore.com/) - Create, play, and print beautiful sheet music
* [picojson](https://github.com/kazuho/picojson) - A header-file-only, JSON parser serializer in C++
* [SDL2](https://www.libsdl.org/index.php) - Simple DirectMedia Layer software development library
* [SDL2_image](https://www.libsdl.org/projects/SDL_image/) - Image file loading library extensions to SDL2
* [SDL2_TTF](https://www.libsdl.org/projects/SDL_ttf/docs/SDL_ttf.html) - TTF Support in SDL
* [SDL2_net](https://www.libsdl.org/projects/SDL_net) - SDL Networking
* [spdlog](https://github.com/gabime/spdlog) - Fast C++ logging library
* [tinytoml](https://github.com/mayah/tinytoml) - A header only C++11 library for parsing TOML
* [vcpkg](https://github.com/Microsoft/vcpkg) - C++ Library Manager for Windows, Linux, and MacOS

## Other Screenshots

7/7/2018 - Modeling player friction

![Split Screen][raptr-0.1-alpha.3]

7/5/2018 - Fluid split screen local multiplayer

![Split Screen][raptr-0.1-alpha.2]

7/4/2018 - A dialog system with branching, decision making, and triggers. Also showing some of the movement.

![Feature Mash][raptr-0.1-alpha.1]

7/2/2018 - Quickly prototype maps by using the alpha channel of a texture to denote collisions.

![Alpha Blending][raptr-alpha]

6/28/2018 - Another look at the dialog system

![Dialog][raptr-dialog]

6/24/2018 - Emotions are tagged in Aseprite's export making it easy to add new ones.

![Emotions][raptr-emotions]

[raptr-idle]: https://i.imgur.com/sqVdbnN.gif
[raptr-alpha]: https://i.imgur.com/Lxa18EC.gif
[raptr-dialog]: https://i.imgur.com/3vOIo3g.gif
[raptr-emotions]: https://i.imgur.com/CP0NvDQ.gif
[raptr-0.1-alpha.1]: https://i.imgur.com/s6YP2qo.gif
[raptr-0.1-alpha.2]: https://thumbs.gfycat.com/AfraidFatalGrison-size_restricted.gif
[raptr-0.1-alpha.3]: https://i.imgur.com/szO854w.gif
[raptr-0.1-alpha.4]: https://i.imgur.com/DQ5CTNO.gif
