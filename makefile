TARGET = dc
VERSION = 0.1

LIBSRC = src/dc.c src/lexer.c
REPLSRC = src/repl.c

CC := cc
RM := rm -f

CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -Wno-unused-variable -Wno-unused-parameter
CFLAGS += -std=c99 -D_XOPEN_SOURCE -D_DEFAULT_SOURCE
CFLAGS += -O2
CFLAGS += -ggdb
CFLAGS += -D_DC_VERSION=\"$(VERSION)\"

CFLAGS +=  $(shell pkg-config --cflags gmp)
LDFLAGS += $(shell pkg-config --libs   gmp)

LDFLAGS += -lm

all: $(TARGET) lib$(TARGET).a

LIBO = $(notdir $(LIBSRC:.c=.o))

%.o: src/%.c
	$(CC) -c -o $@ $(CFLAGS) $^

$(TARGET): $(LIBO) $(REPLSRC)
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

lib$(TARGET).a: $(LIBO)
	$(AR) rcs lib$(TARGET).a $^ 

clean:
	$(RM) $(LIBO) $(TARGET) lib$(TARGET).a

.PHONY: all clean
