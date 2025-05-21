# Target executable name (without extension)
TARGET      := main
# Sources
SOURCES     := $(TARGET).c
# Objects:
# This creates a list of .o files from .c files.
# e.g. if SOURCES = main.c foo.c, then OBJECTS = main.o foo.o
OBJECTS     := $(SOURCES:.c=.o)

# Path to devkitPro. Adjust if yours is different or use environment variable.
# Assumes DEVKITPRO environment variable is set. If not, uncomment and set:
# DEVKITPRO   := /c/devkitPro
DEVKITPRO   ?= $(shell echo $$DEVKITPRO) # Uses env var if set, otherwise empty (adjust if needed)

# Path to libgba (derived from DEVKITPRO)
LIBGBA      := $(DEVKITPRO)/libgba
GBAINCLUDE  := $(LIBGBA)/include
GBALIB      := $(LIBGBA)/lib

# Compiler and tools
CC          := arm-none-eabi-gcc
AS          := arm-none-eabi-as
LD          := arm-none-eabi-gcc # Using gcc as the linker driver is common
OBJCOPY     := arm-none-eabi-objcopy
GBAFIX      := gbafix

# Compiler flags
# -I for include paths
# -mthumb for Thumb instruction set
# -O2 for optimization (optional, good for release)
# -Wall for all warnings (good for development)
# -g for debug symbols (optional, good for development)
CFLAGS      := -I$(GBAINCLUDE) -mthumb -O2 -Wall -g
ASFLAGS     := -mthumb -g

# Linker flags
# -specs=gba.specs for GBA specific linking
# -L for library paths
# -lgba, -lc, -lgcc for linking GBA, C standard, and GCC helper libraries
LDFLAGS     := -specs=gba.specs -mthumb -L$(GBALIB) -lgba -lc -lgcc

# Default target: builds the .gba file
all: $(TARGET).gba

# Rule to link the .elf file
$(TARGET).elf: $(OBJECTS)
	@echo Linking $@...
	@$(LD) $(OBJECTS) -o $@ $(LDFLAGS)

# Rule to compile .c files to .o files
# $< is the first prerequisite (the .c file)
# $@ is the target (the .o file)
%.o: %.c
	@echo Compiling $<...
	@$(CC) $(CFLAGS) -c $< -o $@

# Rule to create the .gba ROM from the .elf file
$(TARGET).gba: $(TARGET).elf
	@echo Creating $@ from $<...
	@$(OBJCOPY) -O binary $< $@
	@echo Fixing $@...
	@$(GBAFIX) $@
	@echo Build complete: $@

# Clean up build files
clean:
	@echo Cleaning...
	@rm -f $(OBJECTS) $(TARGET).elf $(TARGET).gba $(TARGET).map

# Phony targets are targets that don't represent actual files
.PHONY: all clean