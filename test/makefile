CXX = g++

TARGET = test
SRCS = test.cpp

.PHONY:com
com: $(SRCS)
	@$(CXX) $(SRCS) -o $(TARGET)

.PHONY:run
run:$(TARGET)
	@clear
	@./$(TARGET)

clean:
	rm -f $(TARGET)