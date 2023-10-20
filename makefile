TARGET = bin/main

SRCDIR = src
OBJDIR = obj
INCDIR = include

SRC = $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC))
INC = $(wildcard $(INCDIR)/*.h)

CXX = g++
CXXFLAGS = -std=c++20 -Wall -g

.PHONY: all clean

LIBS =

all: $(TARGET)
	@./$(TARGET)

$(TARGET): $(OBJ)
	@$(CXX) $(CXXFLAGS) -I$(INCDIR) -o $@ $^ $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	@$(CXX) $(CXXFLAGS) -I$(INCDIR) -c -o $@ $<

$(OBJDIR):
	@mkdir -p $(OBJDIR)

clean:
	@rm -rf $(TARGET) $(OBJDIR)

.PHONY: all clean