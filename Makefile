WARNINGS := -Wall -Wextra -Wpedantic -Wno-unused-parameter
LIBS := -lSDL2 -lSDL2_ttf -lm
CFLAGS := $(WARNINGS)

SRCS := $(wildcard source/*.c)
HEADERS := $(wildcard source/*.h)
OBJS := $(SRCS:source/%.c=build/%.o)

all: pixelfish

pixelfish: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

build/%.o: source/%.c $(HEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f pixelfish build/*

.PHONY: all clean
