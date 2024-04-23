SRCS = $(shell echo *.cpp)
OBJS = $(SRCS:%.cpp=%.o)
BIN = libcaptionmod2.so

%.o: %.cpp
	g++ -c -fPIC $(CXXFLAGS) $^ -o $@
%.o: %.c
	gcc -c -fPIC $(CFLAGS) $^ -o $@
$(BIN): $(OBJS)
	g++ -shared -o $@ $^

all: $(BIN)

install: $(BIN)
	cp libcaptionmod2.so /usr/local/lib/