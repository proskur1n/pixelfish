WARNINGS := -Wall -Wextra -Wpedantic -Wno-unused-parameter
LIBS := -lSDL2 -lSDL2_ttf -lSDL2_image -lm
FLAGS := $(WARNINGS)

pixelfish: source/*
	$(CC) $(FLAGS) -o pixelfish source/*.c $(LIBS)

clean:
	rm -f pixelfish
