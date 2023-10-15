TARGET = bin/main

SRCDIR = src

INCDIR = include

SRC = $(wildcard $(SRCDIR)/*.cpp)
INC = $(wildcard $(INCDIR)/*.h)

CXX = g++
CXXFLAGS = -std=c++20 -Wall -g

.PHONY: $(TARGET)

LIBS = 

$(TARGET): $(SRC) $(INC)
	@$(CXX) $(CXXFLAGS) -I$(INCDIR) -o $@ $^ $(LIBS)
	@clear
	@./$(TARGET)

clean:
	rm -f $(TARGET)