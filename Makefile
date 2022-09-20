WARNINGS := -Wall -Wextra -Wpedantic -Wno-unused-parameter
LIBS := -lSDL2 -lm
FLAGS := $(WARNINGS)

all:
	$(CC) $(FLAGS) -o pixelfish source/*.c $(LIBS)

clean:
	rm -f pixelfish

