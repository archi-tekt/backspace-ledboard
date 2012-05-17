CC=gcc
CFLAGS=-Wall -Wextra
TARGET=libedboard.a

OBJS=libedboard.o

$(TARGET): $(OBJS)
	ar rcs $(TARGET) $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

clean:
	-$(RM) $(TARGET) *.o
