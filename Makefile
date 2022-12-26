WARNINGS := -Wall -Wextra -Wpedantic -Wno-unused-parameter
LIBS := -lSDL2 -lSDL2_ttf -lm
FLAGS := $(WARNINGS)

pixelfish: source/*
	$(CC) -o pixelfish $(FLAGS) source/*.c $(LIBS)

clean:
	rm -f pixelfish
