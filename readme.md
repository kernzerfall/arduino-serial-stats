# arduino-serial-stats

Display stats from your PC on an Arduino with a 16x2 LCD

## Compatibility

This is _(for now?)_ only compatible with

* GCC under Linux; or
* MinGW under Windows

The MSVC toolset will **not** compile this.

## Build

### Dependencies

* CMake 3.16+
* GCC/Linux or MinGW/Windows

### Procedure (Linux)

```bash
# Clone the repo
$ git clone https://github.com/kernzerfall/arduino-serial-stats.git
$ cd arduino-serial-stats

# Create your build directory
$ mkdir build
$ cd build

# Build the project
$ cmake ..
$ make
```

### Procedure (Windows)

```cmd
REM Clone the repo
> git clone https://github.com/kernzerfall/arduino-serial-stats.git
> cd arduino-serial-stats

REM Create your build directory
> mkdir build
> cd build

REM Important!! Tell CMake to generate MinGW Makefiles
> cmake -G "MinGW Makefiles" ..

REM Build the project
> mingw32-make
```
