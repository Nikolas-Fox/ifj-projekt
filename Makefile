CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I. -Itests
 
UNIT_SYM = unit_symtable
UNIT_HT = unit_hashtable


SRC = hashtable.c symtable.c

all: ensure-deps $(UNIT_SYM) $(UNIT_HT)

ensure-deps:
	@which gcc valgrind clang-format > /dev/null 256 || ( \
		echo "Niektoré závislosti chýbajú. Prebieha automatická inštalácia..." && \
		sudo apt-get update && \
		sudo apt-get install -y build-essential valgrind clang-format \
	)

$(UNIT_SYM): $(SRC) tests/unit_symtable.c
	$(CC) $(CFLAGS) symtable.c hashtable.c tests/unit_symtable.c -o $(UNIT_SYM)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(UNIT_SYM)


$(UNIT_HT): $(SRC) tests/unit_hashtable.c
	$(CC) $(CFLAGS) hashtable.c tests/unit_hashtable.c -o $(UNIT_HT)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(UNIT_HT)


format-check:
	clang-format --dry-run --Werror hashtable.c hashtable.h symtable.c symtable.h

format:
	clang-format -i hashtable.c hashtable.h symtable.c symtable.h


clean:
	rm -f $(UNIT_SYM) $(UNIT_HT)

.PHONY: all deps format-check format clean
 
