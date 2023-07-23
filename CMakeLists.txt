cmake_minimum_required(VERSION 3.11.0)
project(audioplot VERSION 0.1.0)

##---------------------------------------------------------------------
## OpenGL
##---------------------------------------------------------------------

find_package(OpenGL REQUIRED)

##---------------------------------------------------------------------
## GLFW OPENGL WINDOW/CONTEXT/IO LIBRARY - https://github.com/glfw/glfw.git
##---------------------------------------------------------------------

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

##---------------------------------------------------------------------
## ImGui - https://github.com/ocornut/imgui.git
##---------------------------------------------------------------------

set(IMGUI_HEADERS
    thirdparty/imgui/imconfig.h
    thirdparty/imgui/imgui_impl_glfw.h
    thirdparty/imgui/imgui_impl_opengl3.h
    thirdparty/imgui/imgui_impl_opengl3_loader.h
    thirdparty/imgui/imgui_internal.h
    thirdparty/imgui/imgui.h    
    thirdparty/imgui/imstb_rectpack.h
    thirdparty/imgui/imstb_textedit.h
    thirdparty/imgui/imstb_truetype.h
)

set(IMGUI_SRC
    thirdparty/imgui/imgui_demo.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_impl_glfw.cpp
    thirdparty/imgui/imgui_impl_opengl3.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/imgui.cpp
)

add_library(imgui ${IMGUI_HEADERS} ${IMGUI_SRC})
target_include_directories(imgui PUBLIC ${GLFW_INCLUDE_DIRS})
target_include_directories(imgui PUBLIC thirdparty/imgui)
target_link_directories(imgui PUBLIC ${GLFW_STATIC_LIBRARY_DIRS})
target_link_libraries(imgui PUBLIC ${GLFW_STATIC_LIBRARIES} OpenGL::GL)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS GL_SILENCE_DEPRECATION)

##---------------------------------------------------------------------
## ImPlot - https://github.com/epezent/implot.git
##---------------------------------------------------------------------

set(IMPLOT_HEADERS
    thirdparty/implot/implot.h
    thirdparty/implot/implot_internal.h
)

set(IMPLOT_SRC
    thirdparty/implot/implot.cpp
    thirdparty/implot/implot_items.cpp
) 

add_library(implot ${IMPLOT_HEADERS} ${IMPLOT_SRC})
target_include_directories(implot PUBLIC thirdparty/implot)
target_link_libraries(implot PUBLIC imgui)
set_property(TARGET implot PROPERTY CXX_STANDARD 11)
target_compile_options(implot PRIVATE -Wall -Wextra -pedantic -Werror -O3)

##---------------------------------------------------------------------
## KISS FFT - https://github.com/mborgerding/kissfft
##---------------------------------------------------------------------

set(KISSFFT_HEADERS
    thirdparty/kissfft/kiss_fft.h
    thirdparty/kissfft/kiss_fftr.h
)

set(KISSFFT_SRC
    thirdparty/kissfft/kiss_fft.c
    thirdparty/kissfft/kiss_fftr.c
) 

add_library(kissfft ${KISSFFT_HEADERS} ${KISSFFT_SRC})
target_include_directories(implot PUBLIC thirdparty/kissfft)
set_property(TARGET kissfft PROPERTY C_STANDARD 11)
target_compile_options(kissfft PRIVATE -Wall -Wextra -pedantic -Werror -Wno-newline-eof -O3)

##---------------------------------------------------------------------
## audioplot
##---------------------------------------------------------------------

add_executable(audioplot source/audioplot.cpp)
target_include_directories(audioplot PRIVATE thirdparty/pfd)
target_include_directories(audioplot PRIVATE thirdparty/dr_libs)
target_include_directories(audioplot PRIVATE thirdparty/stb)
target_include_directories(audioplot PRIVATE thirdparty/kissfft)
set(AUDIOPLOT_SRC
    source/audioplot_dr_flac.cpp
    source/audioplot_dr_mp3.cpp
    source/audioplot_dr_wav.cpp
    source/audioplot_kiss_fft.cpp
    source/audioplot_pfd.cpp
    source/audioplot_stb_vorbis.cpp
)
target_sources(audioplot PRIVATE ${AUDIOPLOT_SRC})
set_property(TARGET audioplot PROPERTY CXX_STANDARD 11)
target_compile_options(audioplot PRIVATE -O3 -Wall -Wextra -Wformat)
target_link_libraries(audioplot kissfft implot imgui)
