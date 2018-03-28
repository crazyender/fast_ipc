SRCS = $(wildcard src/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
TARGET = fipc
CFLAGS = -Iinc -Isrc

all: share static

share: $(OBJS)
	gcc -shared -o lib$(TARGET).so $(OBJS)

static: $(OBJS)
	ar rcs lib$(TARGET).a $(OBJS)

%.o: %.c
	gcc $(CFLAGS) -c -fpic -o $@ $^

clean:
	rm -f $(OBJS) lib$(TARGET).*
