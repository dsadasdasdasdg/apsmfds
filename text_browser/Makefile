# Compiler and Linker
CC = gcc
CFLAGS = -Wall -g -std=c11 # Added -std=c11 for modern C features if any
LDFLAGS = -lcurl
IFLAGS = -Isrc

# Source files and Object files
SRCDIR = src
# Automatically find all .c files in SRCDIR
SRCS = $(wildcard $(SRCDIR)/*.c)
# Replace src/ prefix with obj/ prefix for object files, and .c with .o
OBJS = $(patsubst $(SRCDIR)/%.c, obj/%.o, $(SRCS))

# Target executable
TARGET = text_browser
OBJDIR = obj

# Default rule
all: $(TARGET)

# Rule to link the executable
$(TARGET): $(OBJS)
	@echo "Linking..."
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Pattern rule to compile .c files into .o files
# Creates object files in the OBJDIR directory
obj/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

# Rule to create the object directory
$(OBJDIR):
	@mkdir -p $(OBJDIR)

# Clean rule
clean:
	@echo "Cleaning up..."
	rm -f $(TARGET) $(OBJDIR)/*.o
	@rmdir $(OBJDIR) 2>/dev/null || true # Remove obj directory if empty

# Phony targets
.PHONY: all clean
