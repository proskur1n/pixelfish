WARNINGS := -Wall -Wextra -Wpedantic -Wno-unused-parameter
LIBS := -lSDL2 -lSDL2_ttf -lm
FLAGS := $(WARNINGS)

all:
	$(CC) $(FLAGS) -o pixelfish source/*.c $(LIBS)

clean:
	rm -f pixelfish

