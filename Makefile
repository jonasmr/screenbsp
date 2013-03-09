
LDFLAGS=/usr/local/lib/libopenal.a /usr/local/lib/libjansson.a -framework OpenGL -framework CoreAudio `/usr/local/bin/sdl-config --static-libs`
CFLAGS=`/usr/local/bin/sdl-config --cflags` -mmacosx-version-min="10.7" -I../glew/glew-1.9.0/include -DGLEW_STATIC -Wno-c++11-extensions
CFLAGS+=-g -O0 -Wno-invalid-offsetof

CPP_SOURCES = 	main.cpp math.cpp coli.cpp render.cpp world.cpp text.cpp \
				sprite.cpp debugdraw.cpp ai.cpp projectile.cpp texture.cpp \
				light.cpp gametime.cpp skills.cpp player.cpp baseclass.cpp \
				fx.cpp enemy.cpp damage.cpp classdesc.cpp worldevent.cpp \
				bigbitmapobject.cpp gui.cpp audio.cpp loot.cpp \
				pfind.cpp inventory.cpp level.cpp loader.cpp dialog.cpp openglshit.cpp \
				network.cpp socket_osx.cpp enumhelper.cpp upgrades.cpp \
				buildmode.cpp threading_osx.cpp gamesync.cpp\
				
C_SOURCES = stb_image.c glew.c
CC=clang
CPP=clang++
LD=clang++
CPP_OBJS = $(patsubst %.cpp,%.o,$(CPP_SOURCES))
C_OBJS = $(patsubst %.c,%.o,$(C_SOURCES))


all: slasher


slasher: $(C_OBJS) $(CPP_OBJS)
	$(LD) -o slasher $(C_OBJS) $(CPP_OBJS) $(LDFLAGS)

-include .depend

.cpp.o: 
	$(CPP) -c $< $(CFLAGS) -o $@

.c.o:
	$(CC) -c $< $(CFLAGS) -o $@


clean: depend
	rm *.o slasher

depend: $(CPP_SOURCES) $(C_SOURCES)
	$(CC) -MM $(CFLAGS) $^ > .depend