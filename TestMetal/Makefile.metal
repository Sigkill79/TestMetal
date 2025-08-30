# Makefile for Engine Metal Library Testing
# This allows testing the Metal engine library without the full Xcode project

CC = gcc
OBJC = clang
CFLAGS = -std=c99 -O2 -march=native -ffast-math -Wall -Wextra
OBJCFLAGS = -fobjc-arc -framework Foundation -framework Metal -framework MetalKit -framework ModelIO -framework MetalPerformanceShaders
LDFLAGS = -lm -framework Foundation -framework Metal -framework MetalKit -framework ModelIO -framework MetalPerformanceShaders

# Source files
METAL_SOURCES = engine_metal.m engine_metal_test.c
METAL_OBJECTS = engine_metal.o engine_metal_test.o

# Targets
all: metal_test

metal_test: $(METAL_OBJECTS)
	$(OBJC) $(METAL_OBJECTS) -o metal_test $(LDFLAGS)

engine_metal.o: engine_metal.m
	$(OBJC) $(OBJCFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Test the Metal engine library
test: metal_test
	./metal_test

# Clean up
clean:
	rm -f *.o metal_test

# Install Metal engine library (copy to system)
install: engine_metal.h engine_metal.c
	sudo cp engine_metal.h /usr/local/include/
	sudo cp engine_metal.c /usr/local/lib/
	sudo ln -sf /usr/local/lib/engine_metal.c /usr/local/lib/libenginemetal.a

.PHONY: all test clean install
