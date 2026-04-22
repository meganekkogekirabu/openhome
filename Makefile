CC := clang
CFLAGS := -Wall -Wextra -Werror -std=c23

ifdef DEBUG
CFLAGS += -g
endif

DEPS := avahi-client
CFLAGS += $(shell pkgconf --cflags $(DEPS))
LDFLAGS += $(shell pkgconf --libs $(DEPS))

SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:src/%.c=build/objects/%.o)
TARGET := build/openhome

$(TARGET): $(OBJECTS)
	clang $^ -o $@ $(LDFLAGS)
	
$(OBJECTS): build/objects/%.o : src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm $(TARGET)
	rm $(OBJECTS)
