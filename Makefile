SRCS = $(wildcard src/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
TEST_FILE = $(wildcard test/*.c)
TESTS = $(patsubst test/%.c,%.test,$(TEST_FILE))
TARGET = fipc
CFLAGS := -Iinc -Isrc -L`pwd` -DNDEBUG
ifeq ($(shell uname),Darwin)
	CFLAGS += -I/usr/local/opt/openssl/include
	LDFLAGS = -L/usr/local/opt/openssl/lib -lpthread -lcrypto
else
	LDFLAGS = -lrt -lpthread -lcrypto
endif

all: clean share static test

share: $(OBJS)
	gcc -shared -o lib$(TARGET).so $(OBJS)

static: $(OBJS)
	ar rcs lib$(TARGET).a $(OBJS)

%.o: %.c
	gcc $(CFLAGS) -c -fpic -o $@ $^

%.test: test/%.c
	gcc $(CFLAGS) -o $@ $^ -l$(TARGET) $(LDFLAGS)

clean:
	rm -f $(OBJS) lib$(TARGET).* $(TESTS)

test: $(TESTS)

debug: clean
	$(MAKE) -C . all CFLAGS="$(CFLAGS) -g"
