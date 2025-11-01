# Makefile for ASCII metablobs
# inspired by a1kon's obfuscated c contest donut

CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lm

# Targets
TARGETS = metaballs

.PHONY: all clean run run-clean run-metaballs

# Build all targets
all: $(TARGETS)

# Build metaballs
metaballs: metaballs.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Run metaballs 
run-metaballs: metaballs
	./metaballs

# Clean build artifacts
clean:
	rm -f $(TARGETS)

# Help target
help:
	@echo "Available targets:"
	@echo "  all           - Build all programs (default)"
	@echo "  metaballs     - Build metaballs animation"
	@echo "  run-metaballs - Build and run metaballs"
	@echo "  clean         - Remove compiled binaries"
	@echo "  help          - Show this help message"
