CC  = gcc
SRC = dxball_game.c
CFLAGS =

UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(OS),Windows_NT)
    OUT  = dxball.exe
    LIBS = -lraylib -lopengl32 -lgdi32 -lwinmm
else ifeq ($(UNAME_S),Darwin)
    OUT  = dxball
    RAYLIB_PREFIX := $(shell brew --prefix raylib 2>/dev/null)
    CFLAGS = -I$(RAYLIB_PREFIX)/include
    LIBS = -L$(RAYLIB_PREFIX)/lib -lraylib -lm -ldl -lpthread -framework OpenGL -framework Cocoa -framework IOKit
else
    OUT  = dxball
    LIBS = -lraylib -lm -ldl -lpthread -lGL -lrt -lX11
endif

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS)

clean:
	rm -f dxball dxball.exe highscores.txt
