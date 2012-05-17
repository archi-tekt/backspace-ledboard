CC=gcc
CFLAGS=-Wall -Wextra -I../libedboard
LFLAGS=-L../libedboard -ledboard
TARGET=ledmatrix

$(TARGET): ledmatrix.o
	$(CC) $< -o $@ $(LFLAGS)

ledmatrix.o: ledmatrix.c
	$(CC) $(CFLAGS) -c $<

clean:
	-$(RM) $(TARGET) *.o
