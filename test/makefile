CXX = g++

TARGET = test02
SRCS = test02.cpp

.PHONY:com
com: $(SRCS)
	@$(CXX) $(SRCS) -o $(TARGET)

.PHONY:run
run:$(TARGET)
	@clear
	@./$(TARGET)

clean:
	rm -f $(TARGET)