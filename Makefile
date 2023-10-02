CFLAGS=-Werror -Wextra -Og
SRC=$(wildcard src/*.c)

all: bin/basalt

bin/:
	mkdir -p bin/

bin/basalt: $(SRC) | bin/
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -r bin/

.PHONY: install
install: bin/basalt
	cp bin/basalt /usr/bin/basalt
