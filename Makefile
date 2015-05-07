# Building the tcp-metrics tool

CC=gcc
CFLAGS=-I.
CCFLAGS=-Wall

ODIR=obj

_OBJ = main.o measure_tcp.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c
	test -d $(ODIR) || mkdir $(ODIR)
	$(CC) $(CCFLAGS) -c -o $@ $< $(CFLAGS)

tcp-metrics: $(OBJ)
	$(CC) $(CCFLAGS) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o
