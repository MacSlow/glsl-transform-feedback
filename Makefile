CXX = clang++

APP_DEBUG   = transform-feedback_debug
APP_RELEASE = transform-feedback_release

CXXFLAGS  = -DGL_GLEXT_PROTOTYPES -Wall -Werror -Ofast -DRELEASE -std=c++11 -pedantic `sdl2-config --cflags` `pkg-config --cflags SDL2_image glew`
CXXFLAGSD = -DGL_GLEXT_PROTOTYPES -Wall -Werror -ggdb -std=c++11 -pedantic -pg `sdl2-config --cflags` `pkg-config --cflags SDL2_image glew`
LIBS      = `sdl2-config --libs` `pkg-config --libs SDL2_image glew` -lGL
LIBSD     = `sdl2-config --libs` `pkg-config --libs SDL2_image glew` -lGL -pg

SRCS = transform-feedback.cpp utils.cpp

OBJS_RELEASE = $(SRCS:.cpp=_r.o)

OBJS_DEBUG = $(SRCS:.cpp=_d.o)

all: $(APP_DEBUG) $(APP_RELEASE)
debug: $(APP_DEBUG)
release: $(APP_RELEASE)

%_r.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%_d.o: %.cpp
	$(CXX) $(CXXFLAGSD) -c $< -o $@

$(APP_DEBUG): $(OBJS_DEBUG)
	$(CXX) -o $@ $^ $(LIBSD)

$(APP_RELEASE): $(OBJS_RELEASE)
	$(CXX) -o $@ $^ $(LIBS)
	strip $@

clean:
	rm -f *_r.o *_d.o $(APP_DEBUG) $(APP_RELEASE) *~
