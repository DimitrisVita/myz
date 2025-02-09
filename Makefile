CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = myz
SRCDIR = src
INCDIR = include
OBJS = $(SRCDIR)/main.o $(SRCDIR)/myz.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(SRCDIR)/main.o: $(SRCDIR)/main.c $(INCDIR)/myz.h
	$(CC) $(CFLAGS) -I$(INCDIR) -c $(SRCDIR)/main.c -o $(SRCDIR)/main.o

$(SRCDIR)/myz.o: $(SRCDIR)/myz.c $(INCDIR)/myz.h
	$(CC) $(CFLAGS) -I$(INCDIR) -c $(SRCDIR)/myz.c -o $(SRCDIR)/myz.o

clean:
	rm -f $(TARGET) $(OBJS)
