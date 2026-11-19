

# AI-generated(Gemini): Here is my simple Makefile and repository structure. 
# Update this Makefile with minimal changes so that the project can ru on a 
# fresh machine with all required dependencies automatically installed when missing.

CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I. -Itests

TARGET   = compiler
MAIN_SRC = main.c
CORE_SRC = ast.c codegen.c hashtable.c parser.c precedence.c scanner.c semantic.c symtable.c

UNIT_SYM = unit_symtable
UNIT_HT  = unit_hashtable
SRC      = hashtable.c symtable.c

build: $(TARGET)

$(TARGET): $(MAIN_SRC) $(CORE_SRC)
	$(CC) $(CFLAGS) $(MAIN_SRC) $(CORE_SRC) -o $(TARGET)

install-clang-format:
	@command -v clang-format > /dev/null 2>&1 || ( \
		echo "clang-format nenájdený. Inštalujem..." && \
		sudo apt-get update && \
		sudo apt-get install -y clang-format \
	)

check: install-clang-format format-check unit-tests

build-check: check build

unit-tests: $(UNIT_SYM) $(UNIT_HT)

$(UNIT_SYM): $(SRC) tests/unit_symtable.c
	$(CC) $(CFLAGS) symtable.c hashtable.c tests/unit_symtable.c -o $(UNIT_SYM)

$(UNIT_HT): $(SRC) tests/unit_hashtable.c
	$(CC) $(CFLAGS) hashtable.c tests/unit_hashtable.c -o $(UNIT_HT)

format-check:
	clang-format --dry-run --Werror hashtable.c hashtable.h symtable.c symtable.h

format: install-clang-format
	clang-format -i hashtable.c hashtable.h symtable.c symtable.h

clean:
	rm -f $(TARGET) $(UNIT_SYM) $(UNIT_HT)

.PHONY: build install-clang-format check build-check unit-tests format-check format clean