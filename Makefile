BINARYDIR := build
TARGET := gshtcpd

CC := clang-3.5
LD := clang-3.5
CFLAGS := -Wall
LDFLAGS := $(CFLAGS)

# Will set INCLUDE_DIRS variable.
include makefiles/include_dir.mak

CFLAGS += $(addprefix -I,$(INCLUDE_DIRS))

# Will set SOURCES variable.
include makefiles/source.mak

OBJECTS := $(addprefix $(BINARYDIR)/, $(SOURCES:.c=.o))

$(BINARYDIR)/%.o: %.c
	@mkdir -p $(BINARYDIR)/$(shell dirname $^)
	$(CC) $(CFLAGS) -c $^ -o $@

$(BINARYDIR)/$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -rf $(BINARYDIR)
