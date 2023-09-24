CXX = g++

TARGET = RiderPart
SRCS = RiderPart.cpp


$(TARGET): $(SRCS)
	@$(CXX) $(SRCS) -o $(TARGET)

.PHONY:run
run:$(TARGET)
	@clear
	@./$(TARGET)

clean:
	rm -f $(TARGET)