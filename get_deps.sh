#! /bin/bash

##---------------------------------------------------------------------
## Clone third-party repositories to deps/
##---------------------------------------------------------------------
mkdir -p deps/

pushd deps > /dev/null
if [ ! -d imgui ]; then
    git clone https://github.com/ocornut/imgui.git
fi

if [ ! -d implot ]; then
    git clone https://github.com/epezent/implot.git
fi

if [ ! -d portable-file-dialogs ]; then
    git clone https://github.com/samhocevar/portable-file-dialogs.git
fi

if [ ! -d dr_libs ]; then
    git clone https://github.com/mackron/dr_libs.git
fi

if [ ! -d stb ]; then
    git clone https://github.com/nothings/stb.git
fi
popd > /dev/null

##---------------------------------------------------------------------
## Checkout specific git commits
##---------------------------------------------------------------------
pushd deps/imgui > /dev/null
git checkout 21fc57f2  # docking branch tip as of 7/8/2022
popd > /dev/null

pushd deps/implot > /dev/null
git checkout 7a470b2e  # master branch tip as of 7/26/2022
popd > /dev/null

pushd deps/portable-file-dialogs > /dev/null
git checkout 265c5af
popd > /dev/null

pushd deps/dr_libs > /dev/null
git checkout c1f072e
popd > /dev/null

pushd deps/stb > /dev/null
git checkout f54acd4
popd > /dev/null

##---------------------------------------------------------------------
## Copy minimal set of dependencies from deps/ to thirdparty/
##---------------------------------------------------------------------
rm -rf thirdparty
mkdir -p thirdparty/

mkdir -p thirdparty/imgui
cp deps/imgui/backends/imgui_impl_glfw.cpp \
   deps/imgui/backends/imgui_impl_glfw.h \
   deps/imgui/backends/imgui_impl_opengl3_loader.h \
   deps/imgui/backends/imgui_impl_opengl3.cpp \
   deps/imgui/backends/imgui_impl_opengl3.h \
   deps/imgui/*.cpp \
   deps/imgui/*.h \
   deps/imgui/LICENSE.txt \
   thirdparty/imgui

mkdir -p thirdparty/implot
cp deps/implot/implot.cpp \
   deps/implot/implot_items.cpp \
   deps/implot/implot_internal.h \
   deps/implot/implot.h \
   deps/implot/LICENSE \
   thirdparty/implot

mkdir -p thirdparty/dr_libs
cp deps/dr_libs/dr_mp3.h \
   deps/dr_libs/dr_wav.h \
   deps/dr_libs/README.md \
   thirdparty/dr_libs

mkdir -p thirdparty/stb
cp deps/stb/stb_vorbis.c \
   deps/stb/LICENSE \
   thirdparty/stb

mkdir -p thirdparty/pfd
cp deps/portable-file-dialogs/portable-file-dialogs.h \
   deps/portable-file-dialogs/COPYING \
   thirdparty/pfd

mkdir -p thirdparty/glfw
if [[ `uname` =~ "MINGW64" ]]; then
    ## Download precompiled 64-bit Windows binaries
    if [ ! -f deps/glfw-3.3.2.bin.WIN64.zip ]; then
        curl -L -o deps/glfw-3.3.2.bin.WIN64.zip https://github.com/glfw/glfw/releases/download/3.3.2/glfw-3.3.2.bin.WIN64.zip
    fi
    if [ ! -d deps/glfw-3.3.2.bin.WIN64 ]; then
        unzip deps/glfw-3.3.2.bin.WIN64.zip -d deps/
    fi

    cp -R deps/glfw-3.3.2.bin.WIN64/include/GLFW \
          thirdparty/glfw/
    cp -R deps/glfw-3.3.2.bin.WIN64/lib-mingw-w64 \
          thirdparty/glfw
    cp deps/glfw-3.3.2.bin.WIN64/LICENSE.md \
        thirdparty/glfw
fi

##---------------------------------------------------------------------
## Modify imgui's default imconfig.h to enable 32-bit indices
##---------------------------------------------------------------------
cp thirdparty/imgui/imconfig.h thirdparty/imgui/imconfig_orig.h
sed 's/\/\/#define ImDrawIdx/#define ImDrawIdx/' thirdparty/imgui/imconfig_orig.h > thirdparty/imgui/imconfig.h
rm -f thirdparty/imgui/imconfig_orig.h

echo "thirdparty dependencies updated"
echo "ready to run make and build audioplot.exe"
