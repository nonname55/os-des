CXX = g++

TARGET = test01
SRCS = test01.cpp

.PHONY:com
com: $(SRCS)
	@$(CXX) $(SRCS) -o $(TARGET)

.PHONY:run
run:$(TARGET)
	@clear
	@./$(TARGET)

clean:
	rm -f $(TARGET)