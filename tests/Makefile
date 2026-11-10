# Host build of the pure game logic (no devkitPro needed).
CC ?= cc
CFLAGS := -std=gnu11 -Wall -Wextra -Wno-unused-parameter -O1 -I../include
SRC := test_logic.c $(wildcard ../source/game/*.c)

test_logic: $(SRC) $(wildcard ../include/game/*.h)
	$(CC) $(CFLAGS) -o $@ $(SRC) -lm

run: test_logic
	./test_logic

.PHONY: run
