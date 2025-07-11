#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need SDL2 (http://www.libsdl.org):
# Linux:
#   apt-get install libsdl2-dev
# Mac OS X:
#   brew install sdl2
# MSYS2:
#   pacman -S mingw-w64-i686-SDL2
#

#CXX = g++
#CXX = clang++

EXE = monitor
OPTIMIZED_EXE = optimized_monitor
IMGUI_DIR = imgui/lib/
SOURCES = main.cpp
SOURCES += system.cpp
SOURCES += mem.cpp
SOURCES += network.cpp
SOURCES += enhanced.cpp
SOURCES += config_export.cpp
SOURCES += enhanced_ui_polished.cpp
SOURCES += ui_polish.cpp

# Optimized version sources
OPTIMIZED_SOURCES = optimized_main.cpp
OPTIMIZED_SOURCES += optimized_implementation.cpp
OPTIMIZED_SOURCES += optimized_rendering.cpp
OPTIMIZED_SOURCES += optimized_readers.cpp
OPTIMIZED_SOURCES += system.cpp
OPTIMIZED_SOURCES += mem.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backend/imgui_impl_sdl.cpp $(IMGUI_DIR)/backend/imgui_impl_opengl3.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

# Optimized version objects
OPTIMIZED_SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
OPTIMIZED_SOURCES += $(IMGUI_DIR)/backend/imgui_impl_sdl.cpp $(IMGUI_DIR)/backend/imgui_impl_opengl3.cpp
OPTIMIZED_SOURCES += imgui/lib/gl3w/GL/gl3w.c
OPTIMIZED_OBJS = $(addsuffix .o, $(basename $(notdir $(OPTIMIZED_SOURCES))))
UNAME_S := $(shell uname -s)

CXXFLAGS = -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backend
CXXFLAGS += -g -Wall -Wformat -O2 -std=c++17
LIBS =

##---------------------------------------------------------------------
## OPENGL LOADER
##---------------------------------------------------------------------

## Using OpenGL loader: gl3w [default]
SOURCES += imgui/lib/gl3w/GL/gl3w.c
CXXFLAGS += -I imgui/lib/gl3w -DIMGUI_IMPL_OPENGL_LOADER_GL3W

## Using OpenGL loader: glew
## (This assumes a system-wide installation)
# CXXFLAGS += -DIMGUI_IMPL_OPENGL_LOADER_GLEW
# LIBS += -lGLEW

## Using OpenGL loader: glad
# SOURCES += ../libs/glad/src/glad.c
# CXXFLAGS += -I../libs/glad/include -DIMGUI_IMPL_OPENGL_LOADER_GLAD

## Using OpenGL loader: glad2
# SOURCES += ../libs/glad/src/gl.c
# CXXFLAGS += -I../libs/glad/include -DIMGUI_IMPL_OPENGL_LOADER_GLAD2

## Using OpenGL loader: glbinding
## This assumes a system-wide installation
## of either version 3.0.0 (or newer)
# CXXFLAGS += -DIMGUI_IMPL_OPENGL_LOADER_GLBINDING3
# LIBS += -lglbinding
## or the older version 2.x
# CXXFLAGS += -DIMGUI_IMPL_OPENGL_LOADER_GLBINDING2
# LIBS += -lglbinding

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -lGL -ldl `sdl2-config --libs`

	CXXFLAGS += `sdl2-config --cflags`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`
	LIBS += -L/usr/local/lib -L/opt/local/lib

	CXXFLAGS += `sdl2-config --cflags`
	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
   ECHO_MESSAGE = "MinGW"
   LIBS += -lgdi32 -lopengl32 -limm32 `pkg-config --static --libs sdl2`

   CXXFLAGS += `pkg-config --cflags sdl2`
   CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backend/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:imgui/lib/gl3w/GL/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o:imgui/lib/glad/src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

optimized: $(OPTIMIZED_EXE)
	@echo Optimized build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

$(OPTIMIZED_EXE): $(OPTIMIZED_OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS) -lpthread

# Test extractor for validation
test_extractor: test_extractor.o system.o mem.o network.o
	$(CXX) -o $@ $^ $(CXXFLAGS) -lpthread

test_extractor.o: test_extractor.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Performance benchmark  
simple_benchmark: simple_benchmark.o optimized_readers.o
	$(CXX) -o $@ $^ $(CXXFLAGS) -lpthread

simple_benchmark.o: simple_benchmark.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Debug discrepancies tool (simple version)
simple_debug: simple_debug.o optimized_implementation.o optimized_readers.o
	$(CXX) -o $@ $^ $(CXXFLAGS) -lpthread

simple_debug.o: simple_debug.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(EXE) $(OBJS) $(OPTIMIZED_EXE) $(OPTIMIZED_OBJS) test_extractor test_extractor.o
