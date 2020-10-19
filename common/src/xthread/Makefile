CC=g++

HEADER :=$(wildcard *.h)
SOURCES := $(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o,$(SOURCES))

TARGET = libxthread.so

CFLAGS = -std=c++11 -pedantic-errors  -Wall -Wno-deprecated -D _LINUX_COMPILE
LIB_PATHS = 


$(TARGET) : $(OBJS)
	@echo ""
	@echo "[OBJECTS] $(OBJS)"
	@echo ""
	$(CC) -shared -Wl,-soname,$(TARGET) $(CFLAGS) $(LIB_PATHS) -o $(TARGET) $(OBJS) $(USING_LIBS)

%.o: %.cpp $(HEADER)
	@echo ""
	@echo "[COMPILE] $@ --------------------------------------"
	$(CC) -fPIC $(CFLAGS) -c $< -o $@ $(LIB_PATHS)

install:
	@echo ""
	@echo "[INSTALL] ------------------------------------"
	cp -f xthread.h ../../../include
	cp -f $(TARGET) ../../../lib
	cp -f $(TARGET) /usr/local/lib
clean:
	@echo ""
	@echo "[CLEAN] --------------------------------------"
	rm -f $(OBJS) $(TARGET)

