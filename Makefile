CXX = g++
CC = gcc

SOURCES =
INCLUDES =
LIBPATH =
LIBS =

UNAME_S := $(shell uname -s)

##---------------------------------------------------------------------
## Audioplot
##---------------------------------------------------------------------

EXE = audioplot

SOURCES += source/audioplot.cpp
SOURCES += source/audioplot_dr_mp3.cpp
SOURCES += source/audioplot_dr_wav.cpp
SOURCES += source/audioplot_pfd.cpp
SOURCES += source/audioplot_stb_vorbis.cpp
INCLUDES += -Isource/

##---------------------------------------------------------------------
## ImGui - https://github.com/ocornut/imgui.git
##---------------------------------------------------------------------
SOURCES += thirdparty/imgui/imgui.cpp
SOURCES += thirdparty/imgui/imgui_draw.cpp
SOURCES += thirdparty/imgui/imgui_impl_glfw.cpp
SOURCES += thirdparty/imgui/imgui_impl_opengl3.cpp
SOURCES += thirdparty/imgui/imgui_widgets.cpp
INCLUDES += -Ithirdparty/imgui/ -DIMGUI_IMPL_OPENGL_LOADER_GL3W

##---------------------------------------------------------------------
## GL3W - https://github.com/skaslev/gl3w (generated files bundled with imgui)
##---------------------------------------------------------------------
SOURCES += thirdparty/gl3w/GL/gl3w.c
INCLUDES += -Ithirdparty/gl3w/

##---------------------------------------------------------------------
## ImPlot - https://github.com/epezent/implot.git
##---------------------------------------------------------------------
SOURCES += thirdparty/implot/implot.cpp
SOURCES += thirdparty/implot/implot_items.cpp
INCLUDES += -Ithirdparty/implot/

##---------------------------------------------------------------------
## Portable File Dialogs - https://github.com/samhocevar/portable-file-dialogs.git
##---------------------------------------------------------------------
INCLUDES += -Ithirdparty/pfd/
ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
	LIBS += -lcomdlg32
	LIBPATH += -L"/c/Program Files (x86)/Windows Kits/10/Lib/10.0.16299.0/um/x64"
endif

##---------------------------------------------------------------------
## DR LIBS - https://github.com/mackron/dr_libs.git
##---------------------------------------------------------------------
INCLUDES += -Ithirdparty/dr_libs/

##---------------------------------------------------------------------
## STB - https://github.com/nothings/stb.git
##---------------------------------------------------------------------
INCLUDES += -Ithirdparty/stb/

##---------------------------------------------------------------------
## GLFW OPENGL WINDOW/CONTEXT/IO LIBRARY - https://github.com/glfw/glfw.git
##---------------------------------------------------------------------

ifeq ($(UNAME_S),Linux)
    LIBS += $(shell pkg-config --static --libs glfw3)
	LIBS += -lGL
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
	LIBS += -lglfw3 -lopengl32 -lglu32 -lgdi32
	LIBPATH += -Lthirdparty/glfw/lib-mingw-w64
	INCLUDES += -Ithirdparty/glfw/
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

CFLAGS = -O3 -std=c11 -Wall -Wextra -Wformat
CXXFLAGS = -O3 -std=c++11 -Wall -Wextra -Wformat

OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

%.o:source/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

%.o:thirdparty/imgui/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

%.o:thirdparty/implot/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

%.o:thirdparty/gl3w/GL/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(LIBPATH) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
