CC = g++
CFLAGS = -Wall -Wextra -std=c++17 -Ofast -march=native -flto
#-DENABLE_STATS -march=native -flto main.cpp -o simulator

TARGET = programme
SRC = main_test.cpp fonctions.cpp task2.cpp
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

main.o: main_test.cpp fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

fonctions.o: fonctions.cpp fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
