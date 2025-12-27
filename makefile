TARGET = dc
VERSION = 0.1
SRC = src/dc.c src/lexer.c

CC := cc

CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -std=c99 -D_XOPEN_SOURCE -D_DEFAULT_SOURCE
CFLAGS += -O2
CFLAGS += -ggdb
CFLAGS += -D_DC_VERSION=\"$(VERSION)\"

CFLAGS +=  $(shell pkg-config --cflags gmp)
LDFLAGS += $(shell pkg-config --libs   gmp)

LDFLAGS += 

$(TARGET): $(SRC)
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)
