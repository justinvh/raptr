## Manually Fetching Dependencies with [vcpkg](https://github.com/Microsoft/vcpkg)

You will want to first install [vcpkg from Microsoft](https://github.com/Microsoft/vcpkg). Their README.md makes it very simple to get going. Once installed, continue on.

| Dependency     | Why it is a dependency |                                
| -------------- | ------------ |                                
| bzip2          | File archives and compression |               
| catch2         | Unit testing |                                
| crossguid      | GUID for entities in the game |               
| cxxopts        | Command-line parsing |                        
| discord-rpc    | Integration with Discord |                    
| doxygen        | Documentation |
| fmt            | Used with spdlog for .format() like strings|  
| freetype       | Font support in the UI |
| libjpeg-turbo  | Texture support |
| liblzma        | Texture support |
| libpng         | Texture support |
| libwebp        | Texture support |
| lua            | Game component scripting |
| picojson       | JSON parsing (for aseprite) |
| sdl2           | Window and event abstractions |
| sdl2-image     | Image loading abstractions |
| sdl2-mixer     | Cross-platform sound |
| sdl2-net       | Cross-platform UDP setup |
| sol2           | C++-Lua helpers |
| spdlog         | Beautifully simple logging |
| tiff           | Texture support |
| tinytoml       | Dead simple TOML config parsing |
| zlib           | It's pretty much standard everywhere |
                                                   
### 1. Patch fmt package

The fmt package is backwards incompatible with spdlog at the moment, so you will have to checkout the port
at a specific version:

```
$ cd %USERPROFILE%\vcpkg
$ git checkout 178517052f42d428bb2f304946e635d3c1f318e9 -- ports/fmt
```

### 2. Download all dependencies

Notice that this is a `x64-windows` triplet.

```
$ %USERPROFILE%\vcpkg\vcpkg.exe --triplet x64-windows install discord-rpc sdl2 sdl2-image sdl2-mixer sdl2-net sdl2-ttf picojson cxxopts spdlog tinytoml catch2 crossguid lua sol2 doxygen
```

If you are building documentation, then you will need [doxygen] and [dot].

### 3. Point CMake to the Toolchain file

Now you can specify the toolchain file:

```
$ git clone https://github.com/justinvh/raptr.git raptr
$ cd raptr/build
$ cmake -DCMAKE_TOOLCHAIN_FILE:PATH="%USERPROFILE%/vcpkg/scripts/buildsystems/vcpkg.cmake" -G "Visual Studio 15 2017 Win64" ..
```

And launch the solution. Build with Visual Studio!

[doxygen]: http://www.doxygen.nl/download.html
[dot]: https://graphviz.gitlab.io/download/
