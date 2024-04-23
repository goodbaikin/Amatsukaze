CFLAGS = -I../include_gpl -DHAVE_STDINT_H -DHAVE_MEMCPY -I.
SRCS = $(shell echo *.c)
OBJS = $(SRCS:%.c=%.o)
BIN = libfaad.so

%.o: %.cpp
	g++ -c -fPIC $(CXXFLAGS) $^ -o $@
%.o: %.c
	gcc -c -fPIC $(CFLAGS) $^ -o $@
$(BIN): $(OBJS)
	g++ -shared -o $@ $^

all: $(BIN)
install: $(BIN)
	cp libfaad.so /usr/local/lib/