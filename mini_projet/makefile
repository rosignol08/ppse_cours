CC = g++
CFLAGS = -Wall -Wextra -std=c++17 -Ofast

TARGET = programme
SRC = main.cpp fonctions.cpp
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.cpp fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

fonctions.o: fonctions.cpp fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
