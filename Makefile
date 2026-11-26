CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I.

TARGET   = compiler
MAIN_SRC = main.c
CORE_SRC = ast.c codegen.c hashtable.c parser.c precedence.c scanner.c semantic.c symtable.c

FORMAT_FILES = $(wildcard *.c *.h)
E2E_SCRIPT   = ./tests/e2e_tests/e2e/run_e2e.sh

build: install-cc $(TARGET)

$(TARGET): $(MAIN_SRC) $(CORE_SRC)
	$(CC) $(CFLAGS) $(MAIN_SRC) $(CORE_SRC) -o $(TARGET)

install-cc:
	@command -v $(CC) > /dev/null 2>&1 || ( \
		echo "$(CC) nenájdený. Inštalujem..." && \
		sudo apt-get update && \
		sudo apt-get install -y build-essential \
	)

install-clang-format:
	@command -v clang-format > /dev/null 2>&1 || ( \
		echo "clang-format nenájdený. Inštalujem..." && \
		sudo apt-get update && \
		sudo apt-get install -y clang-format \
	)

check: install-clang-format format-check

build-check: check build

tests: build
	$(E2E_SCRIPT)

format-check:
	clang-format --dry-run --Werror $(FORMAT_FILES)

format: install-clang-format
	clang-format -i $(FORMAT_FILES)

clean:
	rm -f $(TARGET)

.PHONY: build install-cc install-clang-format check build-check tests format-check format clean