# vim: noexpandtab:

CC := clang
CFLAGS := -Wall -Wextra -Werror -std=c23

ifdef DEBUG
CFLAGS += -g
endif

C := src/main.c

TARGET := build/openhome

OBJECTS = $(C:src/%.c=build/objects/%.o) \
          $(SWIFT:src/%.swift=build/swift/%.o)

PLATFORM := $(shell uname -s)

ifeq ($(PLATFORM), Linux)
  C += src/discovery/linux.c
  DEPS += avahi-client
  CFLAGS += $(shell pkgconf --cflags $(DEPS))
  LDFLAGS += $(shell pkgconf --libs $(DEPS))

else ifeq ($(PLATFORM), Darwin)
  SWIFT += src/discovery/darwin.swift
  SWIFTFLAGS += -import-objc-header src/discovery/Discovery-Bridging-Header.h
  LDFLAGS += \
    -framework CoreFoundation \
    -framework SystemConfiguration \
    -framework Foundation

else
  $(error unsupported platform: $(PLATFORM))

endif

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)
	
build/objects/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/swift/%.o: src/%.swift
	@mkdir -p $(dir $@)
	swiftc $(SWIFTFLAGS) -parse-as-library -c $< -o $@

.PHONY: clean

clean:
	rm $(TARGET)
	rm $(OBJECTS)
