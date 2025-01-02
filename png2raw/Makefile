export CC=${CROSS}gcc

.PHONY: all
all:
	${CC} main.c -o png2raw -lSDL -lSDL_image

.PHONY: clean
clean:
	rm -rf png2raw
