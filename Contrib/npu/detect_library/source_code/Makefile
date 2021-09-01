
CROSS_COMPILE=aarch64-linux-gnu-

LIB_DIR=$(CC)/libc/lib

INCLUDE += -I./include
CFLAGS += $(INCLUDE)

################################################################################
# Supply necessary libraries.
LIBS +=-L$(LIB_DIR) -l stdc++  -l m

CFLAGS +=-I. -O2 -fPIE -Wall -pthread $(LIBS)
ODIR=obj
OFILE=libnn_detect.so

_OBJ= detect.o
_OBJ += detect_log.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(OFILE): $(OBJ)
	#$(CC) -shared -o $(OFILE) ./obj/*.o
	$(CC) $(CFLAGS) -shared -o $(OFILE) ./obj/*.o

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(OFILE)
