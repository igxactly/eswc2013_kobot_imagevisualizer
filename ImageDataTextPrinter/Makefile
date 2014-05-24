CC=/usr/local/arm-linux-4.1.1/bin/arm-iwmmxt-linux-gnueabi-gcc

CFLAGS += -mcpu=iwmmxt -mtune=iwmmxt -mabi=aapcs-linux	\
		-I./ -I./lib/include -DDEBUG_BUILD

LDFLAGS += -lpthread -lm

SRC = main.c modules/color.c lib/camera.c lib/overlay2.c
OBJ = ${SRC:.c=.o}

OUT = imageDataTextPrinter.bin
BUILDDIR = bin/

$(OUT): $(OBJ)
	mkdir -p $(BUILDDIR)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(BUILDDIR)$(OUT) $(LIBS)

clean: 
	rm -f $(BUILDDIR)$(OUT) *.o *.?~ modules/*.o modules/*.?~ lib/*.o lib/*.?~ 
