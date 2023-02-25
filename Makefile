CFLAGS=-Werror -Wextra -Og
SRC=$(wildcard src/*.c)

all: bin/bs

bin/:
	mkdir -p bin/

bin/bs: $(SRC) | bin/
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -r bin/

.PHONY: install
install: bin/bs
	cp bin/bs /usr/bin/bs
