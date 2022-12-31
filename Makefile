WARNINGS := -Wall -Wextra -Wpedantic -Wno-unused-parameter
LIBS := -lSDL2 -lSDL2_ttf -lm `pkg-config --libs gtk+-3.0`
CFLAGS := $(WARNINGS) -Ibuild -g

SRCS := $(wildcard source/*.c) build/embed.c
HEADERS := $(wildcard source/*.h) build/embed.h
OBJS := $(subst source,build,$(patsubst %.c,%.o,$(SRCS)))

# debug build
all: CFLAGS += -DDEVELOPER -fno-omit-frame-pointer
all: pixelfish

release: CFLAGS += -O3 -DNDEBUG
release: pixelfish

pixelfish: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

build/embed.c: build/embed.h
build/embed.h: assets/*
	cd assets && python3 include.py && mv embed.c embed.h ../build

build/embed.o: build/embed.c
	$(CC) $(CFLAGS) -c $< -o $@

build/dialog.o: source/dialog.c source/dialog.h
	@mkdir -p build
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-3.0` -c $< -o $@

build/%.o: source/%.c $(HEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f pixelfish build/* assets/embed.c assets/embed.h

.PHONY: all release clean
