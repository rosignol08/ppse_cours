CC = g++
CFLAGS = -Wall -Wextra -std=c++17 -Ofast -march=native -flto
#-DENABLE_STATS -march=native -flto main.cpp -o simulator

TARGET1 = programme
TARGET2 = programme_travail
COMMON_SRC = fonctions.cpp travail.cpp
SRC1 = main.cpp $(COMMON_SRC)
SRC2 = main_test.cpp $(COMMON_SRC)
OBJ1 = $(SRC1:.cpp=.o)
OBJ2 = $(SRC2:.cpp=.o)

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJ1)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET2): $(OBJ2)
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.cpp fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

main_test.o: main_test.cpp fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

fonctions.o: fonctions.cpp fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

travail.o: travail.cpp fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ1) $(OBJ2) $(TARGET1) $(TARGET2)

.PHONY: all clean
