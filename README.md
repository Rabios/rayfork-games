# rayfork-games

Full port of [raylib sample games](https://github.com/raysan5/raylib-games/blob/master/classics) to [rayfork](https://github.com/SasLuca/rayfork)!

### Building with CMake

Each game in a folder comes with `CMakeLists.txt`, Use CMake to build in each folder from source.

### Common issue

rayfork doesn't have functions to get screen width and height, So they replaced with window width and height instead!

### This repository uses

1. [rayfork](https://github.com/SasLuca/rayfork), Main game programming library.
2. [SasLuca/rayfork-sokol-template](https://github.com/SasLuca/rayfork-sokol-template), Simple rayfork project template with sokol-app and CMake.
3. [sokol-app](https://github.com/floooh/sokol/blob/master/sokol_app.h) for the platform layer.
4. [sokol-time](https://github.com/floooh/sokol/blob/master/sokol_time.h) for high resolution timing.
5. [glad](https://glad.dav1d.de) for OpenGL loading.