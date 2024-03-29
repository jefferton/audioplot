# Audioplot

Audioplot is a simple cross-platform waveform viewer for audio files.

![screenshot1](screenshot1.png)
![screenshot2](screenshot2.png)
![screenshot3](screenshot3.png)

## Usage

Open a file browser to select a file:

    audioplot.exe

Open a supported audio file in the audioplot gui:

    audioplot.exe song.mp3
    audioplot.exe song.wav
    audioplot.exe song.ogg
    audioplot.exe song.flac

## Keyboard Controls

    Esc Key                          --> Exit audioplot
    Arrow Keys                       --> Scroll Cursor Slow (Up/Down) or Fast (Left/Right)
    W/A/S/D keys                     --> Horizontal Pan (A/D) and Zoom (W/S)
    Q/E keys                         --> Vertical Zoom (Q/E)
    F key                            --> Vertical Zoom to Fit Data
    R key                            --> Reset Vertical Zoom
    Space Bar                        --> Reset Pan and Horizontal + Vertical Zoom
    Tab Key                          --> Switch Plot Modes (Combined, Split, Multiple)
    Number Keys (12345667890)        --> Toggle Exclusive View of Channel 1-10
    Shift + Number Keys              --> Toggle Exclusive View of Channel 11-20
    Ctrl + Number Keys               --> Show/Hide Channel 1-10
    Ctrl + Shift + Number Keys       --> Show/Hide Channel 11-20
    Backtick Key (`)                 --> Show All Channels

## Mouse Controls

    Scroll Up/Down                   --> Zoom In/Out
    Middle Click-and-Drag Left/Right --> Move Cursor
    Left Click-and-Drag Left/Right   --> Pan Left/Right

## Building

### Windows

Audioplot has been built and tested successfully on 64-bit Windows 10 with mingw-w64
and GCC 9.2.0.

### Linux

Audioplot has been built and tested successfully on 64-bit Linux Mint 19 with GCC
7.5.0.

You may need to install `xorg-dev` and/or `libglfw3-dev` packages to build successfully.

    sudo apt-get install xorg-dev
    sudo apt-get install libglfw3-dev

### Mac

Audioplot has been built and tested successfully with Apple clang version 13.1.6
on macOS 12.5 running on Apple Silicon.

You may need to install `glfw` to build successfully.

    brew install glfw

## CMake

Audioplot has been build with CMake on llvm clang version 14.0.6 on macOS 13.1
running on Apple Silicon

    mkdir build
    cd build
    CC=/opt/homebrew/opt/llvm/bin/clang CXX=/opt/homebrew/opt/llvm/bin/clang++ cmake ..
    cmake --build .

You may need to install `glfw` and `llvm` to build successfully.

    brew install glfw
    brew install llvm

### Third-Party Dependencies

The necessary third-party files for building audioplot have been copied from their
original repository into the `thirdparty` subdirectory.

The `get_deps.sh` script clones the appropriate third-party repositories, so they are
available for development, for example, to incorporate upstream changes.

### Third-Party Licenses

- dr_libs/README.md                (public domain)
- gl3w/GL/glcorearb.h              (MIT license)
- imgui/LICENSE.txt                (MIT license)
- implot/LICENSE                   (MIT license)
- portable-file-dialogs/COPYING    (WTFPL)
- stb/LICENSE                      (MIT license/public domain)
- kissfft/BSD-3-Clause             (BSD 3-Clause)
