CC=gcc
CFLAGS=-Wall -Wextra -I../libedboard
TARGET=ledloard

$(TARGET): ledloard.c
	$(CC) $(CFLAGS) $< -o $(TARGET)

clean:
	-$(RM) $(TARGET)
