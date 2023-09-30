CXX = g++

TARGET = main
SRCS = main.cpp

.PHONY:com
com: $(SRCS)
	@$(CXX) $(SRCS) -o $(TARGET)

.PHONY:run
run:$(TARGET)
	@clear
	@./$(TARGET)

clean:
	rm -f $(TARGET)