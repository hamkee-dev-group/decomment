CC ?= cc
CFLAGS ?= -O2
CFLAGS += -std=c17 -Wall -Wextra -Wpedantic
CFLAGS += -Iinclude -Isrc

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man/man1

SRCS = src/main.c src/cli.c src/buf.c src/log.c src/platform.c src/fs.c src/lang.c \
       src/handlers/c_like.c src/handlers/python.c src/handlers/shell.c \
       src/handlers/hash.c src/handlers/lua.c

OBJS = $(SRCS:.c=.o)

TEST_SRCS = tests/test_main.c src/buf.c \
            src/handlers/c_like.c src/handlers/python.c src/handlers/shell.c \
            src/handlers/hash.c src/handlers/lua.c

TARGET = decomment
TEST_TARGET = tests/test_runner

.PHONY: all clean test install uninstall lint sanitize

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	$(CC) $(CFLAGS) -DTESTING -o $@ $(TEST_SRCS)

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_TARGET)
	rm -f tests/test_runner

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

lint:
	@echo "Running cppcheck (if available)..."
	-cppcheck --enable=all --std=c17 -Iinclude -Isrc --suppress=missingIncludeSystem src/ 2>&1
	@echo "Running clang-tidy (if available)..."
	-clang-tidy src/*.c src/handlers/*.c -- $(CFLAGS) 2>&1

sanitize:
	$(CC) $(CFLAGS) -fsanitize=address,undefined -g -o $(TARGET)_asan $(SRCS)
	$(CC) $(CFLAGS) -fsanitize=address,undefined -g -DTESTING -o $(TEST_TARGET)_asan $(TEST_SRCS)
	./$(TEST_TARGET)_asan
	rm -f $(TARGET)_asan $(TEST_TARGET)_asan

$(OBJS): include/decomment.h include/buf.h include/cli.h include/fs.h \
         include/lang.h include/log.h include/platform.h \
         src/handlers/handlers.h
