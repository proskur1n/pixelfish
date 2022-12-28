WARNINGS := -Wall -Wextra -Wpedantic -Wno-unused-parameter
LIBS := -lSDL2 -lSDL2_ttf -lm `pkg-config --libs gtk+-3.0`
CFLAGS := $(WARNINGS) -g

SRCS := $(wildcard source/*.c)
HEADERS := $(wildcard source/*.h)
OBJS := $(SRCS:source/%.c=build/%.o)

all: pixelfish

pixelfish: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

build/dialog.o: source/dialog.c source/dialog.h
	@mkdir -p build
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-3.0` -c $< -o $@

build/%.o: source/%.c $(HEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f pixelfish build/*

.PHONY: all clean
