
LDFLAGS=-O0 -framework OpenGL `sdl2-config --static-libs` -lBulletDynamics -lBulletCollision -lLinearMath -lpthread
CFLAGS=`sdl2-config --cflags` -I../glew -DGLEW_STATIC -Wno-c++11-extensions -I/usr/local/include/bullet
CFLAGS+=-g -O0 -Wno-invalid-offsetof -msse -msse2 -msse3 -msse4 -Wno-deprecated-declarations
CPPFLAGS=$(CFLAGS)
CPPFLAGS+=-stdlib=libc++ -std=c++11

CPP_SOURCES = main.cpp math.cpp text.cpp input.cpp program.cpp bsp.cpp debug.cpp manipulator.cpp mesh.cpp shader.cpp \
		microprofile.cpp physics.cpp buffer.cpp test.cpp imgui/imgui.cpp
C_SOURCES = stb_image.c glew.c

TARGET=../bsp
CC=clang
CPP=clang++
LD=clang++
CPP_OBJS = $(patsubst %.cpp,%.o,$(CPP_SOURCES))
C_OBJS = $(patsubst %.c,%.o,$(C_SOURCES))


all: $(TARGET)


$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(LD) -o $(TARGET) $(C_OBJS) $(CPP_OBJS) $(LDFLAGS) $(CPPFLAGS)

-include .depend

.cpp.o: 
	$(CPP) -c $< $(CPPFLAGS) -o $@

.c.o:
	$(CC) -c $< $(CFLAGS) -o $@


clean: depend
	rm *.o $(TARGET)

depend: $(CPP_SOURCES) $(C_SOURCES)
	$(CC) -MM $(CFLAGS) $(C_SOURCES) > .depend
	$(CPP) -MM $(CPPFLAGS) $(CPP_SOURCES) >> .depend