
LDFLAGS=-framework OpenGL -framework CoreAudio `/usr/local/bin/sdl-config --static-libs` -lBulletDynamics -lBulletCollision -lLinearMath -lpthread
CFLAGS=`/usr/local/bin/sdl-config --cflags` -I../glew/glew-1.9.0/include -DGLEW_STATIC -Wno-c++11-extensions -I/usr/local/include/bullet
CFLAGS+=-g -O0 -Wno-invalid-offsetof
CPPFLAGS=$(CFLAGS)
CPPFLAGS+=-Wno-lambda-extensions -stdlib=libc++ -std=c++11

CPP_SOURCES = main.cpp math.cpp text.cpp input.cpp program.cpp bsp.cpp debug.cpp manipulator.cpp mesh.cpp shader.cpp \
		microprofile.cpp physics.cpp
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