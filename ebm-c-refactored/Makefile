CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lm
TARGET = test_modular

SRCS = main.c insolation.c atmosphere.c phase_change.c simulation.c
OBJS = $(SRCS:.c=.o)
HEADERS = ebm.h

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
