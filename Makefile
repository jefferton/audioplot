CXX = g++
CC = gcc

SOURCES =
INCLUDES =
DEFINES =
LIBPATH =
LIBS =

UNAME_S := $(shell uname -s)

##---------------------------------------------------------------------
## Audioplot
##---------------------------------------------------------------------

EXE = audioplot

SOURCES += source/audioplot.cpp
SOURCES += source/audioplot_dr_flac.cpp
SOURCES += source/audioplot_dr_mp3.cpp
SOURCES += source/audioplot_dr_wav.cpp
SOURCES += source/audioplot_pfd.cpp
SOURCES += source/audioplot_stb_vorbis.cpp
SOURCES += source/audioplot_kiss_fft.cpp
INCLUDES += -Isource/

##---------------------------------------------------------------------
## ImGui - https://github.com/ocornut/imgui.git
##---------------------------------------------------------------------
SOURCES += thirdparty/imgui/imgui.cpp
SOURCES += thirdparty/imgui/imgui_draw.cpp
SOURCES += thirdparty/imgui/imgui_tables.cpp
SOURCES += thirdparty/imgui/imgui_impl_glfw.cpp
SOURCES += thirdparty/imgui/imgui_impl_opengl3.cpp
SOURCES += thirdparty/imgui/imgui_widgets.cpp
INCLUDES += -Ithirdparty/imgui/
DEFINES += -DIMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS

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
## KISS FFT - https://github.com/mborgerding/kissfft
##---------------------------------------------------------------------
SOURCES += thirdparty/kissfft/kiss_fft.c
SOURCES += thirdparty/kissfft/kiss_fftr.c
INCLUDES += -Ithirdparty/kissfft/

##---------------------------------------------------------------------
## GLFW OPENGL WINDOW/CONTEXT/IO LIBRARY - https://github.com/glfw/glfw.git
##---------------------------------------------------------------------

ifeq ($(UNAME_S),Linux)
	LIBS += $(shell pkg-config --static --libs glfw3)
	LIBS += -lGL
endif

ifeq ($(UNAME_S),Darwin)
	LIBS += $(shell pkg-config --static --libs glfw3)
	INCLUDES += $(shell pkg-config --cflags glfw3)
	LIBS += -framework OpenGL
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
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c -o $@ $<

%.o:thirdparty/imgui/%.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c -o $@ $<

%.o:thirdparty/implot/%.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c -o $@ $<

%.o:thirdparty/kissfft/%.c
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(LIBPATH) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
