CC=gcc
CFLAGS=-Wall -Wextra
TARGET=ledmatrix

$(TARGET): ledmatrix.c
	$(CC) $(CFLAGS) $< -o $(TARGET)

clean:
	-$(RM) $(TARGET)
