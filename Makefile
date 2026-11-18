CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pthread
TSANFLAGS := -std=c++17 -O1 -g -Wall -Wextra -pthread -fsanitize=thread
SRCS   := rw.cpp monitor_rw.cpp control_atomico_rw.cpp verificador.cpp
HDRS   := control_rw.hpp monitor_rw.hpp control_atomico_rw.hpp verificador.hpp
TARGET := rw
.PHONY: all tsan clean
all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)
tsan: $(SRCS) $(HDRS)
	$(CXX) $(TSANFLAGS) $(SRCS) -o $(TARGET)_tsan
clean:
	rm -f $(TARGET) $(TARGET)_tsan *.o

