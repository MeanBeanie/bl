CFLAGS = -std=c11 -Wall -Wextra -ggdb
TARGET = lang

OBJS = $(patsubst %.c, build/%.o, $(wildcard *.c))

$(TARGET): $(OBJS)
	@gcc -o $@ build/*.o $(CFLAGS)
	@echo $@

build/%.o: %.c $(wildcard *.h)
	@gcc -o $@ -c $< $(CFLAGS)
	@echo $@

.PHONY: clean
clean:
	@rm build/*.o
	@rm $(TARGET)
