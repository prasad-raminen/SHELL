# Compiler and flags
CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -g
LDFLAGS =

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = .

# Source files and object files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))
EXECUTABLE = $(BINDIR)/shell.out

# Default target
all: $(EXECUTABLE)

# Rule to build the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Rule to compile source files into object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -rf $(OBJDIR) $(EXECUTABLE)

# Phony targets
.PHONY: all clean




# # Compiler and flags
# CC = gcc
# CFLAGS = -Iinclude -Wall -Wextra -g
# LDFLAGS =

# # Directories
# SRCDIR = src
# INCDIR = include
# OBJDIR = obj
# BINDIR = .

# # Source files and object files
# SOURCES = $(wildcard $(SRCDIR)/*.c)
# OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))
# EXECUTABLE = $(BINDIR)/shell.out

# # Default target
# all: $(EXECUTABLE)

# # Rule to build the executable
# $(EXECUTABLE): $(OBJECTS)
# 	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# # Rule to compile source files into object files
# $(OBJDIR)/%.o: $(SRCDIR)/%.c
# 	@mkdir -p $(OBJDIR)
# 	$(CC) $(CFLAGS) -c $< -o $@

# # Clean up build artifacts
# clean:
# 	rm -rf $(OBJDIR) $(EXECUTABLE)

# # Phony targets
# .PHONY: all clean
