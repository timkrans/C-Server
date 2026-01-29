CC      = clang
CFLAGS  = -Wall -Wextra -Wpedantic -O2
TARGET  = server
SRC     = src/server.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
